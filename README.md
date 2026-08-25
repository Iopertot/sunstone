# Sunstone - Straight Bullshit

A desktop music player built out of annoyance and spite. Frontend is
Svelte 5 because it's fast, backend is Rust because it's fast, and
somewhere in the middle there's a hamster on a wheel who does the
actual work and sometimes chews on the power cable, which is why your
speakers scream once in a while. He's trying his best. Leave him
alone.

> **Status:** Early build by a
> hamster with no union representation. The player transport, the
> theme, and the Last.fm chief keef auto scrobbler might work Everything else is
> two limes and a prayer. See [Shit you aren't getting](#roadmap-shit-you-arent-getting).

## Why a hamster, and not Electron

| A typical Electron player | Sunstone |
|---|---|
| Your app (JS) | Crackhead and a paint brush |
| Node.js runtime | Dave & Buster's coupons |
| Chromium, bundled whole | Hamster on a wheel |
| **Actually good** | **Hot fucking garbage** |

We know. We're keeping it anyway.

## What it (maybe) does

- Reads your mp3/flac/wav/ogg/m4a/opus tags, most of the time,
  correctly
- Scrobbles to Last.fm, assuming the hamster hasn't chewed through
  the ethernet cable this week
- Automatic Love Sosa Scrobble

### last.fm

The crackhead and his tinfoil hat are sending 5g waves to last fm headquarters
to auto scrobble chief keef love sosa to your account


## Getting this running

Windows. Fuck your other OS.

```bash
git clone https://github.com/Iopertot/sunstone
cd sunstone
npm install
```

You'll also need Rust and the
[Tauri prerequisites](https://v2.tauri.app/start/prerequisites/) for
your OS, because the hamster needs a wheel to run on.

Register an app at <https://www.last.fm/api/account/create> for an
API key + secret. Run it once to generate a config file:

```bash
npm run tauri dev
```

then quit and drop your key + secret into the generated
`config.json` (path is under your OS's app-config dir, identifier
`moe.local.sunstone`). Run `npm run tauri dev` again. There's no
"Connect Last.fm" button in Settings yet, so you're calling
`lastfm_start_auth` / `lastfm_complete_auth` from the frontend
yourself for now. Point `scan_library` at a folder full of music and
the hamster will (probably) find it.

## Roadmap: shit you aren't getting

| Maybe eventually | Never getting |
|---|---|
| Watermelon Gum | A good FLAC player |
| Pictures of my gay cousin | A girlfriend |
| Idk what to put here | A volume control bar |
| A free cameo from gaming4hope | Music that isn't Chief Keef — Love Sosa |

## License

None. I got a DUI.

---

`sunstone.lopertot.com`  built with hatred, no affiliation to the
Chinese government, and no plans to fix any of this.
