//! Last.fm scrobbling.
//!
//! The API itself is small; the reliability comes from three rules most
//! quick integrations skip:
//!
//! 1. `track.updateNowPlaying` and `track.scrobble` are different calls.
//!    Now-playing is fire-and-forget UI candy; the scrobble is the durable
//!    record and is what actually needs to survive.
//! 2. A scrobble only qualifies once the track has played for at least
//!    half its duration or 4 minutes, whichever is LOWER, and only for
//!    tracks longer than 30s. The timestamp submitted is when the track
//!    STARTED, not when it crossed the threshold.
//! 3. Nothing here should live only in memory. Every scrobble is written
//!    to a local sqlite queue the moment it's eligible, and a background
//!    loop retries anything unsent - so a dropped connection, a sleeping
//!    machine, or the app being closed mid-track doesn't just lose it,
//!    which is the single most common failure mode in player scrobblers.

use anyhow::{anyhow, Result};
use rusqlite::Connection;
use serde::{Deserialize, Serialize};
use std::collections::BTreeMap;
use std::sync::Mutex;
use std::time::{SystemTime, UNIX_EPOCH};

const API_ROOT: &str = "https://ws.audioscrobbler.com/2.0/";
const BATCH_SIZE: usize = 50;

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct ScrobbleTrack {
	pub artist: String,
	pub title: String,
	pub album: String,
	pub duration_secs: u32,
	pub started_at: i64,
}

/// updateNowPlaying doesn't need a start timestamp - it's a live "here's
/// what's on" ping, not the durable record.
#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct NowPlayingTrack {
	pub artist: String,
	pub title: String,
	pub album: String,
	pub duration_secs: u32,
}

pub struct LastfmClient {
	http: reqwest::Client,
	api_key: String,
	api_secret: String,
	/// None until the user completes the browser auth flow. Persisted to
	/// disk after that - Last.fm session keys don't expire, so this is a
	/// one-time setup, not a per-launch login.
	session_key: Mutex<Option<String>>,
}

impl LastfmClient {
	pub fn new(api_key: String, api_secret: String, session_key: Option<String>) -> Self {
		Self {
			http: reqwest::Client::new(),
			api_key,
			api_secret,
			session_key: Mutex::new(session_key),
		}
	}

	pub fn has_session(&self) -> bool {
		self.session_key.lock().unwrap().is_some()
	}

	fn sign(&self, params: &BTreeMap<&str, String>) -> String {
		// Last.fm's signing scheme: concatenate sorted key+value pairs
		// (excluding `format`), append the shared secret, md5 it.
		let mut raw = String::new();
		for (k, v) in params {
			if *k == "format" {
				continue;
			}
			raw.push_str(k);
			raw.push_str(v);
		}
		raw.push_str(&self.api_secret);
		format!("{:x}", md5::compute(raw.as_bytes()))
	}

	async fn call(&self, mut params: BTreeMap<&str, String>) -> Result<serde_json::Value> {
		params.insert("api_key", self.api_key.clone());
		let sig = self.sign(&params);
		params.insert("api_sig", sig);
		params.insert("format", "json".into());

		let resp = self.http.post(API_ROOT).form(&params).send().await?;
		let status = resp.status();
		let body: serde_json::Value = resp.json().await?;

		if !status.is_success() || body.get("error").is_some() {
			return Err(anyhow!("last.fm error: {body}"));
		}
		Ok(body)
	}

	// --- Auth (desktop "web auth" flow) ------------------------------

	/// Step 1: get a token, then send the user to the returned URL in
	/// their browser to approve access.
	pub async fn get_auth_token(&self) -> Result<String> {
		let mut params = BTreeMap::new();
		params.insert("method", "auth.getToken".to_string());
		let body = self.call(params).await?;
		Ok(body["token"].as_str().unwrap_or_default().to_string())
	}

	pub fn auth_url(&self, token: &str) -> String {
		format!(
			"https://www.last.fm/api/auth/?api_key={}&token={}",
			self.api_key, token
		)
	}

	/// Step 2: after the user approves in-browser, exchange the token for
	/// a permanent session key and persist it.
	pub async fn complete_auth(&self, token: &str) -> Result<String> {
		let mut params = BTreeMap::new();
		params.insert("method", "auth.getSession".to_string());
		params.insert("token", token.to_string());
		let body = self.call(params).await?;
		let key = body["session"]["key"]
			.as_str()
			.ok_or_else(|| anyhow!("no session key in response"))?
			.to_string();
		*self.session_key.lock().unwrap() = Some(key.clone());
		Ok(key)
	}

	pub fn session_key(&self) -> Option<String> {
		self.session_key.lock().unwrap().clone()
	}

	// --- Live calls -----------------------------------------------------

	pub async fn update_now_playing(&self, t: &NowPlayingTrack) -> Result<()> {
		let sk = self
			.session_key()
			.ok_or_else(|| anyhow!("not authenticated with last.fm"))?;
		let mut params = BTreeMap::new();
		params.insert("method", "track.updateNowPlaying".to_string());
		params.insert("artist", t.artist.clone());
		params.insert("track", t.title.clone());
		params.insert("album", t.album.clone());
		params.insert("duration", t.duration_secs.to_string());
		params.insert("sk", sk);
		self.call(params).await?;
		Ok(())
	}

	/// Submit up to BATCH_SIZE scrobbles in one call, per the API's
	/// array-parameter convention (artist[0], artist[1], ...).
	async fn scrobble_batch(&self, tracks: &[ScrobbleTrack]) -> Result<()> {
		let sk = self
			.session_key()
			.ok_or_else(|| anyhow!("not authenticated with last.fm"))?;

		let mut params: BTreeMap<String, String> = BTreeMap::new();
		params.insert("method".into(), "track.scrobble".into());
		params.insert("sk".into(), sk);
		for (i, t) in tracks.iter().enumerate() {
			params.insert(format!("artist[{i}]"), t.artist.clone());
			params.insert(format!("track[{i}]"), t.title.clone());
			params.insert(format!("album[{i}]"), t.album.clone());
			params.insert(format!("timestamp[{i}]"), t.started_at.to_string());
			params.insert(format!("duration[{i}]"), t.duration_secs.to_string());
		}
		// call() takes &str keys; rebuild with leaked-safe str refs is
		// awkward with dynamic keys, so duplicate the minimal signing
		// logic here instead of reusing call().
		let mut full = params.clone();
		full.insert("api_key".into(), self.api_key.clone());
		let mut raw = String::new();
		for (k, v) in &full {
			if k == "format" {
				continue;
			}
			raw.push_str(k);
			raw.push_str(v);
		}
		raw.push_str(&self.api_secret);
		let sig = format!("{:x}", md5::compute(raw.as_bytes()));
		full.insert("api_sig".into(), sig);
		full.insert("format".into(), "json".into());

		let resp = self.http.post(API_ROOT).form(&full).send().await?;
		let status = resp.status();
		let body: serde_json::Value = resp.json().await?;
		if !status.is_success() || body.get("error").is_some() {
			return Err(anyhow!("last.fm scrobble batch error: {body}"));
		}
		Ok(())
	}
}

// --- Durable queue -------------------------------------------------------

pub fn init_queue_db(conn: &Connection) -> Result<()> {
	conn.execute(
		"CREATE TABLE IF NOT EXISTS scrobble_queue (
			id INTEGER PRIMARY KEY AUTOINCREMENT,
			artist TEXT NOT NULL,
			title TEXT NOT NULL,
			album TEXT NOT NULL,
			duration_secs INTEGER NOT NULL,
			started_at INTEGER NOT NULL,
			attempts INTEGER NOT NULL DEFAULT 0
		)",
		[],
	)?;
	Ok(())
}

pub fn enqueue(conn: &Connection, t: &ScrobbleTrack) -> Result<()> {
	conn.execute(
		"INSERT INTO scrobble_queue (artist, title, album, duration_secs, started_at)
		 VALUES (?1, ?2, ?3, ?4, ?5)",
		rusqlite::params![t.artist, t.title, t.album, t.duration_secs, t.started_at],
	)?;
	Ok(())
}

/// Called on an interval from main.rs. Pulls a batch, submits it, and only
/// removes rows that actually succeeded - anything else stays queued with
/// its attempt count bumped for exponential-ish backoff by the caller.
pub async fn flush_queue(conn: &Mutex<Connection>, client: &LastfmClient) -> Result<usize> {
	if !client.has_session() {
		return Ok(0);
	}

	let rows: Vec<(i64, ScrobbleTrack)> = {
		let conn = conn.lock().unwrap();
		let mut stmt = conn.prepare(
			"SELECT id, artist, title, album, duration_secs, started_at
			 FROM scrobble_queue ORDER BY started_at ASC LIMIT ?1",
		)?;
		let iter = stmt.query_map([BATCH_SIZE as i64], |row| {
			Ok((
				row.get::<_, i64>(0)?,
				ScrobbleTrack {
					artist: row.get(1)?,
					title: row.get(2)?,
					album: row.get(3)?,
					duration_secs: row.get::<_, i64>(4)? as u32,
					started_at: row.get(5)?,
				},
			))
		})?;
		iter.collect::<std::result::Result<Vec<_>, _>>()?
	};

	if rows.is_empty() {
		return Ok(0);
	}

	let tracks: Vec<ScrobbleTrack> = rows.iter().map(|(_, t)| t.clone()).collect();
	match client.scrobble_batch(&tracks).await {
		Ok(()) => {
			let conn = conn.lock().unwrap();
			for (id, _) in &rows {
				conn.execute("DELETE FROM scrobble_queue WHERE id = ?1", [id])?;
			}
			Ok(rows.len())
		}
		Err(e) => {
			let conn = conn.lock().unwrap();
			for (id, _) in &rows {
				conn.execute(
					"UPDATE scrobble_queue SET attempts = attempts + 1 WHERE id = ?1",
					[id],
				)?;
			}
			Err(e)
		}
	}
}

#[allow(dead_code)]
pub fn now_unix() -> i64 {
	SystemTime::now()
		.duration_since(UNIX_EPOCH)
		.unwrap()
		.as_secs() as i64
}
