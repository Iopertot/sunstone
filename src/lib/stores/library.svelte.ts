import { invoke } from '@tauri-apps/api/core';

export interface TrackMeta {
	path: string;
	title: string;
	artist: string;
	album: string;
	duration_secs: number;
}

/**
 * Thin wrapper around the `scan_library` / `list_tracks` Tauri commands so
 * the Library view and the Settings folder picker share one source of
 * truth instead of each keeping their own copy that can drift out of sync.
 */
export const library = (() => {
	let tracks = $state<TrackMeta[]>([]);
	let scanning = $state(false);
	let lastScanError = $state<string | null>(null);

	async function refresh() {
		try {
			tracks = await invoke<TrackMeta[]>('list_tracks');
		} catch (err) {
			console.error('failed to list tracks', err);
		}
	}

	async function scan(root: string) {
		scanning = true;
		lastScanError = null;
		try {
			await invoke<number>('scan_library', { root });
			await refresh();
		} catch (err) {
			lastScanError = String(err);
			console.error('failed to scan library', err);
		} finally {
			scanning = false;
		}
	}

	return {
		get tracks() {
			return tracks;
		},
		get scanning() {
			return scanning;
		},
		get lastScanError() {
			return lastScanError;
		},
		refresh,
		scan
	};
})();
