// Prevents an extra terminal window on Windows in release builds.
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

mod lastfm;
mod library;

use lastfm::{LastfmClient, NowPlayingTrack, ScrobbleTrack};
use library::TrackMeta;
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
}

fn config_path(app: &tauri::AppHandle) -> std::path::PathBuf {
	let dir = app.path().app_config_dir().expect("no config dir");
	fs::create_dir_all(&dir).ok();
	dir.join("config.json")
}

fn load_config(app: &tauri::AppHandle) -> Config {
	let path = config_path(app);
	fs::read_to_string(path)
		.ok()
		.and_then(|s| serde_json::from_str(&s).ok())
		.unwrap_or_default()
}

fn save_config(app: &tauri::AppHandle, cfg: &Config) {
	let path = config_path(app);
	if let Ok(json) = serde_json::to_string_pretty(cfg) {
		let _ = fs::write(path, json);
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

// --- Commands exposed to the Svelte frontend ------------------------------

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
async fn scan_library(state: State<'_, AppState>, root: String) -> Result<usize, String> {
	let conn = state.db.lock().unwrap();
	library::scan_folder(&conn, &root).map_err(|e| e.to_string())
}

#[tauri::command]
async fn list_tracks(state: State<'_, AppState>) -> Result<Vec<TrackMeta>, String> {
	let conn = state.db.lock().unwrap();
	library::list_tracks(&conn).map_err(|e| e.to_string())
}

fn main() {
	tauri::Builder::default()
		.plugin(tauri_plugin_shell::init())
		.plugin(tauri_plugin_dialog::init())
		.setup(|app| {
			let handle = app.handle().clone();
			let cfg = load_config(&handle);

			// Registering an app at https://www.last.fm/api/account/create
			// gives you an api_key/shared secret - drop them in the config
			// file at app_config_dir/config.json (or wire a settings UI
			// that writes it via save_config).
			let client = LastfmClient::new(cfg.api_key, cfg.api_secret, cfg.session_key);

			let db = Connection::open(db_path(&handle)).expect("failed to open db");
			lastfm::init_queue_db(&db).expect("failed to init scrobble queue table");
			library::init_library_db(&db).expect("failed to init library table");

			app.manage(AppState {
				client,
				db: Mutex::new(db),
			});

			// Background retry loop: every 30s, try to flush anything
			// still sitting in the queue. This is what makes scrobbling
			// durable across dropped connections, sleep, or a track that
			// finished while the window was closed.
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
			list_tracks
		])
		.run(tauri::generate_context!())
		.expect("error while running sunstone");
}
