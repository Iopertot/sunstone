# Sunstone

A personal, non-commercial music player in the spirit of [Geoxor's
Amethyst](https://github.com/Geoxor/Amethyst) — same idea (a proper
desktop player built around a Web Audio node graph), but:

- **Svelte 5 + SvelteKit** instead of Vue
- **Tauri (Rust)** instead of Electron — smaller, and it lets the Last.fm
  logic run as a real background service instead of code tied to a UI
  component's lifecycle
- **Last.fm scrobbling that's actually durable** — see below
- **Sunstone theme** — see `src/app.css` for the rationale, not just hex
  codes

This is a starting skeleton, not a finished clone: the player transport,
theme system, library scanner, and — the part you specifically asked
for — the full Last.fm auth/scrobble/retry pipeline are real and wired
end to end. The library browser UI, queue/playlists, and the node-based
DSP chain (EQ, etc.) are left as an empty content pane for you to build
out next.

## Why the scrobbling should actually work this time

Three rules the Last.fm API cares about, implemented in
`src-tauri/src/lastfm.rs`:

1. `track.updateNowPlaying` (live status) and `track.scrobble` (the
   permanent record) are separate calls — both are implemented.
2. A track only qualifies once it's played **≥50% of its duration or 4
   minutes, whichever is lower**, and only if it's longer than 30s
   (`src/lib/stores/player.ts`, `armScrobble`).
3. Every eligible scrobble is written to a local sqlite queue
   immediately (`scrobble_queue` table) and a background loop in Rust
   retries it every 30s until Last.fm accepts it — in batches of up to
   50, per the API's array-parameter convention. This is what survives
   a dropped connection, the machine sleeping, or the app being closed
   mid-track, which is the usual way scrobblers quietly lose plays.

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

## What's next

Good next slices, roughly in order:
- A Settings view wiring the Last.fm connect flow and folder picker
  (`@tauri-apps/plugin-dialog` is already a dependency)
- The library grid/list reading from `list_tracks`
- A visualizer reading `player.analyser` (an `AnalyserNode`, already
  wired into the audio graph) — this is where Amethyst's look lives
- Queue/playlist state, persisted to sqlite alongside the queue

If you want to keep building this iteratively across sessions, Claude
Code (desktop or terminal) is a better fit than chat for the rest of
this — it can run `cargo build`/`npm run tauri dev` directly and keep
the whole repo in context.
