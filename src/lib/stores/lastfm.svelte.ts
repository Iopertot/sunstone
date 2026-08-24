import { invoke } from '@tauri-apps/api/core';
import { open } from '@tauri-apps/plugin-shell';

/**
 * Drives the two-step Last.fm "web auth" flow described in the README:
 * 1. startAuth() gets a token from Rust, opens the approval URL in the
 *    user's real browser (not the webview - Last.fm's login page expects
 *    that), and holds onto the token.
 * 2. Once the user has approved access in the browser, finishAuth()
 *    exchanges that token for a permanent session key, which Rust then
 *    persists to config.json.
 *
 * Shared between the Settings view (where the connect flow lives) and the
 * Now Scrobbling view (which just reads `connected`).
 */
export const lastfm = (() => {
	let connected = $state(false);
	let checked = $state(false);
	let pendingToken = $state<string | null>(null);
	let busy = $state(false);
	let error = $state<string | null>(null);

	async function refreshStatus() {
		try {
			connected = await invoke<boolean>('lastfm_status');
		} catch (err) {
			console.error('failed to check last.fm status', err);
		} finally {
			checked = true;
		}
	}

	async function startAuth() {
		busy = true;
		error = null;
		try {
			const [token, url] = await invoke<[string, string]>('lastfm_start_auth');
			pendingToken = token;
			await open(url);
		} catch (err) {
			error = String(err);
		} finally {
			busy = false;
		}
	}

	async function finishAuth() {
		if (!pendingToken) return;
		busy = true;
		error = null;
		try {
			await invoke('lastfm_complete_auth', { token: pendingToken });
			pendingToken = null;
			await refreshStatus();
		} catch (err) {
			error = String(err);
		} finally {
			busy = false;
		}
	}

	return {
		get connected() {
			return connected;
		},
		get checked() {
			return checked;
		},
		get pendingToken() {
			return pendingToken;
		},
		get busy() {
			return busy;
		},
		get error() {
			return error;
		},
		refreshStatus,
		startAuth,
		finishAuth
	};
})();
