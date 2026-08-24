# Sunstone

A personal, non-commercial music player made out of pure hatred and spite. Built with **Svelte 5 + SvelteKit** for the
UI and **Tauri (Rust)** for everything else audio graph, library
scanner, and a Last.fm scrobbling pipeline.

Same spirit as [Geoxor's Amethyst](https://github.com/Geoxor/Amethyst) a proper desktop player built around a Web Audio node graph with
a different stack and a scrobbler that's actually durable.

> **Status:** early skeleton, not a finished clone. The player
> transport, theme system, library scanner, and the full Last.fm
> auth/scrobble/retry pipeline are real and wired end to end. The
> library browser UI, queue/playlists, and the node-based DSP chain
> (EQ etc.) are still empty panes. See [Roadmap](#roadmap).

## Why Tauri, not Electron

Most desktop music players are a website wearing a window frame a
full copy of Chromium bundled in just to draw a sidebar and a
progress bar. Sunstone uses your OS's own webview instead, and the
things that need to keep running when the window is minimized — the
audio graph, the scrobble queue run as a real Rust background
service rather than code tied to a UI component's lifecycle.

## Scrobbling that's actually durable

Three rules the Last.fm API cares about, implemented in
[`src-tauri/src/lastfm.rs`](src-tauri/src/lastfm.rs):

1. `track.updateNowPlaying` (live status) and `track.scrobble` (the
   permanent record) are separate calls both are implemented.
2. A track only qualifies once it's played **≥50% of its duration or
   4 minutes, whichever is lower**, and only if it's longer than 30s.
3. Every eligible scrobble is written to a local sqlite queue
   immediately (`scrobble_queue` table), and a background loop
   retries it every 30 seconds in batches of up to 50, per the
   API's array-parameter convention until Last.fm accepts it. This
   is what survives a dropped connection, the machine sleeping, or
   the app closing mid-track, which is the usual way scrobblers
   quietly lose plays.

## Features

- Local library scan (`mp3`, `flac`, `wav`, `ogg`, `m4a`, `opus`) via
  [`lofty`](https://crates.io/crates/lofty) tag reading, stored in a
  bundled sqlite database
- Web Audio playback graph with an `AnalyserNode` already wired in
  for a future visualizer
- Last.fm OAuth-style auth flow, now-playing updates, and a
  crash/offline-durable scrobble queue
- Warm, dark **Sunstone** theme see
  [`src/app.css`](src/app.css) for the full rationale, not just hex
  codes

## Stack

| Layer | Tech |
|---|---|
| UI | Svelte 5, SvelteKit, TypeScript |
| Shell | Tauri 2 (Rust) |
| Audio | Web Audio API |
| Local storage | SQLite via `rusqlite` (bundled) |
| Tag reading | `lofty` |
| HTTP | `reqwest` (rustls) + `tokio` |

## Getting started

1. Install [Rust](https://rustup.rs), [Node.js](https://nodejs.org)
   20+, and the
   [Tauri prerequisites](https://v2.tauri.app/start/prerequisites/)
   for your OS.
2. `npm install`
3. Register an app at
   <https://www.last.fm/api/account/create> to get an API key +
   shared secret.
4. Run the app once so it creates its config directory:
   ```bash
   npm run tauri dev
   ```
   then quit it and edit the generated `config.json` — path printed
   by `app_config_dir`, typically:
   - Linux: `~/.config/moe.local.sunstone/config.json`
   - macOS: `~/Library/Application Support/moe.local.sunstone/config.json`
   - Windows: `%APPDATA%\moe.local.sunstone\config.json`

   Fill in `api_key` and `api_secret`.
5. `npm run tauri dev` again. Call the `lastfm_start_auth` command
   from the frontend (wire a "Connect Last.fm" button in Settings
   not built yet) to get an auth URL, open it, approve access, then
   call `lastfm_complete_auth` with the token to store a permanent
   session key.
6. Point the library scanner (`scan_library` command) at a music
   folder to populate the `tracks` table.

### Scripts

| Command | What it does |
|---|---|
| `npm run dev` | Vite dev server (frontend only) |
| `npm run tauri dev` | Full app, dev mode |
| `npm run build` | Build the frontend |
| `npm run tauri build` | Build the native app bundle |
| `npm run check` | `svelte-kit sync` + `svelte-check` |

## Project structure

```
src/
├── app.css                          theme tokens + rationale
├── app.html
├── routes/
│   ├── +layout.svelte
│   └── +page.svelte                 app shell: sidebar + views + player bar
└── lib/
    ├── stores/
    │   ├── player.svelte.ts         transport, audio graph, scrobble arming
    │   ├── library.svelte.ts
    │   └── lastfm.svelte.ts
    └── components/
        ├── PlayerBar.svelte
        └── views/
            ├── LibraryView.svelte
            ├── PlaylistsView.svelte
            ├── NowScrobblingView.svelte
            └── SettingsView.svelte

src-tauri/
├── src/
│   ├── main.rs                      commands, app state, retry-loop setup
│   ├── lastfm.rs                    auth, now-playing, scrobble queue + retry
│   └── library.rs                   folder scan, tag reading, tracks table
├── capabilities/default.json
└── tauri.conf.json
```

## Roadmap

Roughly in order:

- [ ] Settings view wiring the Last.fm connect flow and folder
      picker (`@tauri-apps/plugin-dialog` is already a dependency)
- [ ] Library grid/list reading from `list_tracks`
- [ ] A visualizer reading `player.analyser` this is where
      Amethyst's look lives
- [ ] Queue/playlist state, persisted to sqlite alongside the
      scrobble queue
- [ ] The node-based DSP chain (EQ, etc.)

## License

Personal, non-commercial project.
