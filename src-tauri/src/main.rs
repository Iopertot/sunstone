#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

mod lastfm;
mod library;
mod playlists;

use lastfm::{LastfmClient, NowPlayingTrack, ScrobbleTrack};
use library::{AlbumArt, TrackMeta};
use playlists::PlaylistSummary;
use rusqlite::Connection;
use serde::{Deserialize, Serialize};
use std::fs;
use std::sync::Mutex;
use std::time::Duration;
use tauri::{Manager, State};

#[derive(Serialize, Deserialize, Default, Clone)]
struct Config {
	api_key: String,
	api_secret: String,
	session_key: Option<String>,
	last_library_folder: Option<String>,
}

fn config_path(app: &tauri::AppHandle) -> std::path::PathBuf {
	let dir = app.path().app_config_dir().expect("no config dir");
	fs::create_dir_all(&dir).ok();
	dir.join("config.json")
}

fn load_config(app: &tauri::AppHandle) -> Config {
	let path = config_path(app);

	match fs::read_to_string(&path)
		.ok()
		.and_then(|s| serde_json::from_str::<Config>(&s).ok())
	{
		Some(cfg) => cfg,
		None => {
			let cfg = Config::default();
			save_config(app, &cfg);
			cfg
		}
	}
}

fn save_config(app: &tauri::AppHandle, cfg: &Config) {
	let path = config_path(app);
	if let Ok(json) = serde_json::to_string_pretty(cfg) {
		if let Err(e) = fs::write(&path, json) {
			eprintln!("failed to write Last.fm config {}: {e}", path.display());
		}
	}
}

struct AppState {
	client: LastfmClient,
	db: Mutex<Connection>,
}

fn db_path(app: &tauri::AppHandle) -> std::path::PathBuf {
	let dir = app.path().app_data_dir().expect("no data dir");
	fs::create_dir_all(&dir).ok();
	dir.join("sunstone.sqlite")
}

#[tauri::command]
async fn lastfm_status(state: State<'_, AppState>) -> Result<bool, String> {
	Ok(state.client.has_session())
}

#[tauri::command]
async fn lastfm_start_auth(state: State<'_, AppState>) -> Result<(String, String), String> {
	let token = state.client.get_auth_token().await.map_err(|e| e.to_string())?;
	let url = state.client.auth_url(&token);
	Ok((token, url))
}

#[tauri::command]
async fn lastfm_complete_auth(
	app: tauri::AppHandle,
	state: State<'_, AppState>,
	token: String,
) -> Result<(), String> {
	state
		.client
		.complete_auth(&token)
		.await
		.map_err(|e| e.to_string())?;

	let mut cfg = load_config(&app);
	cfg.session_key = state.client.session_key();
	save_config(&app, &cfg);
	Ok(())
}

#[tauri::command]
async fn update_now_playing(state: State<'_, AppState>, track: NowPlayingTrack) -> Result<(), String> {
	state
		.client
		.update_now_playing(&track)
		.await
		.map_err(|e| e.to_string())
}

#[tauri::command]
async fn queue_scrobble(state: State<'_, AppState>, track: ScrobbleTrack) -> Result<(), String> {
	let conn = state.db.lock().unwrap();
	lastfm::enqueue(&conn, &track).map_err(|e| e.to_string())
}

#[tauri::command]
async fn scan_library(
	app: tauri::AppHandle,
	state: State<'_, AppState>,
	root: String,
) -> Result<usize, String> {
	let count = {
		let conn = state.db.lock().unwrap();
		library::scan_folder(&conn, &root).map_err(|e| e.to_string())?
	};
	let mut cfg = load_config(&app);
	cfg.last_library_folder = Some(root);
	save_config(&app, &cfg);
	Ok(count)
}

#[tauri::command]
async fn get_last_library_folder(app: tauri::AppHandle) -> Result<Option<String>, String> {
	Ok(load_config(&app).last_library_folder)
}

#[tauri::command]
async fn list_tracks(state: State<'_, AppState>) -> Result<Vec<TrackMeta>, String> {
	let conn = state.db.lock().unwrap();
	library::list_tracks(&conn).map_err(|e| e.to_string())
}

#[tauri::command]
async fn get_album_art(path: String) -> Result<Option<AlbumArt>, String> {
	Ok(library::read_album_art(std::path::Path::new(&path)))
}

#[tauri::command]
async fn create_playlist(state: State<'_, AppState>, name: String) -> Result<i64, String> {
	let conn = state.db.lock().unwrap();
	playlists::create_playlist(&conn, &name).map_err(|e| e.to_string())
}

#[tauri::command]
async fn rename_playlist(
	state: State<'_, AppState>,
	playlist_id: i64,
	name: String,
) -> Result<(), String> {
	let conn = state.db.lock().unwrap();
	playlists::rename_playlist(&conn, playlist_id, &name).map_err(|e| e.to_string())
}

#[tauri::command]
async fn delete_playlist(state: State<'_, AppState>, playlist_id: i64) -> Result<(), String> {
	let conn = state.db.lock().unwrap();
	playlists::delete_playlist(&conn, playlist_id).map_err(|e| e.to_string())
}

#[tauri::command]
async fn list_playlists(state: State<'_, AppState>) -> Result<Vec<PlaylistSummary>, String> {
	let conn = state.db.lock().unwrap();
	playlists::list_playlists(&conn).map_err(|e| e.to_string())
}

#[tauri::command]
async fn add_track_to_playlist(
	state: State<'_, AppState>,
	playlist_id: i64,
	track_path: String,
) -> Result<(), String> {
	let conn = state.db.lock().unwrap();
	playlists::add_track(&conn, playlist_id, &track_path).map_err(|e| e.to_string())
}

#[tauri::command]
async fn remove_track_from_playlist(
	state: State<'_, AppState>,
	playlist_id: i64,
	track_path: String,
) -> Result<(), String> {
	let conn = state.db.lock().unwrap();
	playlists::remove_track(&conn, playlist_id, &track_path).map_err(|e| e.to_string())
}

#[tauri::command]
async fn get_playlist_tracks(
	state: State<'_, AppState>,
	playlist_id: i64,
) -> Result<Vec<TrackMeta>, String> {
	let conn = state.db.lock().unwrap();
	playlists::get_tracks(&conn, playlist_id).map_err(|e| e.to_string())
}

fn main() {
	tauri::Builder::default()
		.plugin(tauri_plugin_shell::init())
		.plugin(tauri_plugin_dialog::init())
		.setup(|app| {
			let handle = app.handle().clone();
			let cfg = load_config(&handle);

			let client = LastfmClient::new(cfg.api_key, cfg.api_secret, cfg.session_key);

			let db = Connection::open(db_path(&handle)).expect("failed to open db");
			lastfm::init_queue_db(&db).expect("failed to init scrobble queue table");
			library::init_library_db(&db).expect("failed to init library table");
			playlists::init_playlists_db(&db).expect("failed to init playlists tables");

			app.manage(AppState {
				client,
				db: Mutex::new(db),
			});

			if let Some(folder) = cfg.last_library_folder.clone() {
				let handle_for_scan = handle.clone();
				tauri::async_runtime::spawn(async move {
					let state = handle_for_scan.state::<AppState>();
					let conn = state.db.lock().unwrap();
					if let Err(e) = library::scan_folder(&conn, &folder) {
						eprintln!("startup library rescan failed: {e}");
					}
				});
			}

			let handle_for_loop = handle.clone();
			tauri::async_runtime::spawn(async move {
				let mut interval = tokio::time::interval(Duration::from_secs(30));
				loop {
					interval.tick().await;
					let state = handle_for_loop.state::<AppState>();
					if let Err(e) = lastfm::flush_queue(&state.db, &state.client).await {
						eprintln!("scrobble queue flush failed (will retry): {e}");
					}
				}
			});

			Ok(())
		})
		.invoke_handler(tauri::generate_handler![
			lastfm_status,
			lastfm_start_auth,
			lastfm_complete_auth,
			update_now_playing,
			queue_scrobble,
			scan_library,
			get_last_library_folder,
			list_tracks,
			get_album_art,
			create_playlist,
			rename_playlist,
			delete_playlist,
			list_playlists,
			add_track_to_playlist,
			remove_track_from_playlist,
			get_playlist_tracks
		])
		.run(tauri::generate_context!())
		.expect("error while running sunstone");
}