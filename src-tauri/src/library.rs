use anyhow::Result;
use lofty::file::{AudioFile, TaggedFileExt};
use lofty::probe::Probe;
use lofty::tag::Accessor;
use rusqlite::Connection;
use serde::Serialize;
use std::path::Path;
use walkdir::WalkDir;

#[derive(Serialize, Clone, Debug)]
pub struct TrackMeta {
	pub path: String,
	pub title: String,
	pub artist: String,
	pub album: String,
	pub duration_secs: u32,
}

const AUDIO_EXTS: &[&str] = &["mp3", "flac", "wav", "ogg", "m4a", "opus"];

pub fn init_library_db(conn: &Connection) -> Result<()> {
	conn.execute(
		"CREATE TABLE IF NOT EXISTS tracks (
			path TEXT PRIMARY KEY,
			title TEXT NOT NULL,
			artist TEXT NOT NULL,
			album TEXT NOT NULL,
			duration_secs INTEGER NOT NULL
		)",
		[],
	)?;
	Ok(())
}

fn read_track_meta(path: &Path) -> Option<TrackMeta> {
	let tagged = Probe::open(path).ok()?.read().ok()?;
	let tag = tagged.primary_tag().or_else(|| tagged.first_tag());
	let properties = tagged.properties();

	let file_stem = path.file_stem()?.to_string_lossy().to_string();

	Some(TrackMeta {
		path: path.to_string_lossy().to_string(),
		title: tag
			.and_then(|t| t.title().map(|s| s.to_string()))
			.unwrap_or(file_stem),
		artist: tag
			.and_then(|t| t.artist().map(|s| s.to_string()))
			.unwrap_or_else(|| "Unknown Artist".into()),
		album: tag
			.and_then(|t| t.album().map(|s| s.to_string()))
			.unwrap_or_else(|| "Unknown Album".into()),
		duration_secs: properties.duration().as_secs() as u32,
	})
}

pub fn scan_folder(conn: &Connection, root: &str) -> Result<usize> {
	let mut count = 0;
	for entry in WalkDir::new(root)
		.into_iter()
		.filter_map(|e| e.ok())
		.filter(|e| e.file_type().is_file())
	{
		let path = entry.path();
		let ext = path
			.extension()
			.and_then(|e| e.to_str())
			.unwrap_or_default()
			.to_lowercase();
		if !AUDIO_EXTS.contains(&ext.as_str()) {
			continue;
		}
		if let Some(meta) = read_track_meta(path) {
			conn.execute(
				"INSERT INTO tracks (path, title, artist, album, duration_secs)
				 VALUES (?1, ?2, ?3, ?4, ?5)
				 ON CONFLICT(path) DO UPDATE SET
					title = excluded.title,
					artist = excluded.artist,
					album = excluded.album,
					duration_secs = excluded.duration_secs",
				rusqlite::params![meta.path, meta.title, meta.artist, meta.album, meta.duration_secs],
			)?;
			count += 1;
		}
	}
	Ok(count)
}

pub fn list_tracks(conn: &Connection) -> Result<Vec<TrackMeta>> {
	let mut stmt =
		conn.prepare("SELECT path, title, artist, album, duration_secs FROM tracks ORDER BY artist, album, title")?;
	let rows = stmt.query_map([], |row| {
		Ok(TrackMeta {
			path: row.get(0)?,
			title: row.get(1)?,
			artist: row.get(2)?,
			album: row.get(3)?,
			duration_secs: row.get::<_, i64>(4)? as u32,
		})
	})?;
	Ok(rows.collect::<std::result::Result<Vec<_>, _>>()?)
}
