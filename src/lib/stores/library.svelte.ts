import { invoke } from '@tauri-apps/api/core';

export interface TrackMeta {
	path: string;
	title: string;
	artist: string;
	album: string;
	duration_secs: number;
}

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
