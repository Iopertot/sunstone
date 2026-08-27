# Sunstone

A personal, non-commercial music player in the spirit of [Geoxor's
Amethyst](https://github.com/Geoxor/Amethyst) same idea (a proper
desktop player built around a Web Audio node graph), but:

- **Svelte 5 + SvelteKit**
- **Tauri (Rust)** instead of Electron
- **Last.fm scrobbling**
- **Sunstone theme** - see `src/app.css`

## Setup

1. Install [Rust](https://rustup.rs), [Node.js](https://nodejs.org)
   (20+), and the [Tauri prerequisites](https://v2.tauri.app/start/prerequisites/)
   for your OS.
2. `npm install`
3. Register an app at <https://www.last.fm/api/account/create> to get
   an API key + shared secret.
4. Run the app once (`npm run tauri dev`) so it creates its config
   dir **and an empty `config.json`**, then quit it and edit the generated
   `config.json` (path printed by `app_config_dir` — typically
   `~/.config/moe.local.sunstone/config.json` on Linux,
   `~/Library/Application Support/moe.local.sunstone/config.json` on
   macOS, `%APPDATA%\moe.local.sunstone\config.json` on Windows) to
   fill in `api_key` and `api_secret`.
5. `npm run tauri dev` again. Call the `lastfm_start_auth` command
   from the frontend (wire a "Connect Last.fm" button in Settings —
   not built yet) to get an auth URL, open it, approve access, then
   call `lastfm_complete_auth` with the token to store a permanent
   session key.
6. Point the library scanner (`scan_library` command) at a music
   folder to populate the sqlite `tracks` table.
