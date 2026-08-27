use crate::library::TrackMeta;
use anyhow::Result;
use rusqlite::Connection;
use serde::Serialize;

#[derive(Serialize, Clone, Debug)]
pub struct PlaylistSummary {
	pub id: i64,
	pub name: String,
	pub track_count: i64,
}

pub fn init_playlists_db(conn: &Connection) -> Result<()> {
	conn.execute(
		"CREATE TABLE IF NOT EXISTS playlists (
			id INTEGER PRIMARY KEY AUTOINCREMENT,
			name TEXT NOT NULL
		)",
		[],
	)?;
	conn.execute(
		"CREATE TABLE IF NOT EXISTS playlist_tracks (
			playlist_id INTEGER NOT NULL,
			track_path TEXT NOT NULL,
			position INTEGER NOT NULL,
			PRIMARY KEY (playlist_id, track_path)
		)",
		[],
	)?;
	Ok(())
}

pub fn create_playlist(conn: &Connection, name: &str) -> Result<i64> {
	conn.execute("INSERT INTO playlists (name) VALUES (?1)", [name])?;
	Ok(conn.last_insert_rowid())
}

pub fn rename_playlist(conn: &Connection, playlist_id: i64, name: &str) -> Result<()> {
	conn.execute(
		"UPDATE playlists SET name = ?2 WHERE id = ?1",
		rusqlite::params![playlist_id, name],
	)?;
	Ok(())
}

pub fn delete_playlist(conn: &Connection, playlist_id: i64) -> Result<()> {
	conn.execute(
		"DELETE FROM playlist_tracks WHERE playlist_id = ?1",
		[playlist_id],
	)?;
	conn.execute("DELETE FROM playlists WHERE id = ?1", [playlist_id])?;
	Ok(())
}

pub fn list_playlists(conn: &Connection) -> Result<Vec<PlaylistSummary>> {
	let mut stmt = conn.prepare(
		"SELECT p.id, p.name, COUNT(pt.track_path)
		 FROM playlists p
		 LEFT JOIN playlist_tracks pt ON pt.playlist_id = p.id
		 GROUP BY p.id, p.name
		 ORDER BY p.name COLLATE NOCASE",
	)?;
	let rows = stmt.query_map([], |row| {
		Ok(PlaylistSummary {
			id: row.get(0)?,
			name: row.get(1)?,
			track_count: row.get(2)?,
		})
	})?;
	Ok(rows.collect::<std::result::Result<Vec<_>, _>>()?)
}

pub fn add_track(conn: &Connection, playlist_id: i64, track_path: &str) -> Result<()> {
	conn.execute(
		"INSERT OR IGNORE INTO playlist_tracks (playlist_id, track_path, position)
		 VALUES (
			?1,
			?2,
			(SELECT COALESCE(MAX(position), -1) + 1 FROM playlist_tracks WHERE playlist_id = ?1)
		 )",
		rusqlite::params![playlist_id, track_path],
	)?;
	Ok(())
}

pub fn remove_track(conn: &Connection, playlist_id: i64, track_path: &str) -> Result<()> {
	conn.execute(
		"DELETE FROM playlist_tracks WHERE playlist_id = ?1 AND track_path = ?2",
		rusqlite::params![playlist_id, track_path],
	)?;
	Ok(())
}

pub fn get_tracks(conn: &Connection, playlist_id: i64) -> Result<Vec<TrackMeta>> {
	let mut stmt = conn.prepare(
		"SELECT t.path, t.title, t.artist, t.album, t.duration_secs
		 FROM playlist_tracks pt
		 JOIN tracks t ON t.path = pt.track_path
		 WHERE pt.playlist_id = ?1
		 ORDER BY pt.position ASC",
	)?;
	let rows = stmt.query_map([playlist_id], |row| {
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
