import { invoke } from '@tauri-apps/api/core';
import type { TrackMeta } from './library.svelte';

export interface PlaylistSummary {
	id: number;
	name: string;
	track_count: number;
}

export const playlists = (() => {
	let items = $state<PlaylistSummary[]>([]);
	let loading = $state(false);

	async function refresh() {
		loading = true;
		try {
			items = await invoke<PlaylistSummary[]>('list_playlists');
		} catch (err) {
			console.error('failed to list playlists', err);
		} finally {
			loading = false;
		}
	}

	async function create(name: string): Promise<number> {
		const id = await invoke<number>('create_playlist', { name });
		await refresh();
		return id;
	}

	async function rename(playlistId: number, name: string) {
		await invoke('rename_playlist', { playlistId, name });
		await refresh();
	}

	async function remove(playlistId: number) {
		await invoke('delete_playlist', { playlistId });
		await refresh();
	}

	async function addTrack(playlistId: number, trackPath: string) {
		await invoke('add_track_to_playlist', { playlistId, trackPath });
		await refresh();
	}

	async function removeTrack(playlistId: number, trackPath: string) {
		await invoke('remove_track_from_playlist', { playlistId, trackPath });
	}

	async function getTracks(playlistId: number): Promise<TrackMeta[]> {
		return invoke<TrackMeta[]>('get_playlist_tracks', { playlistId });
	}

	return {
		get items() {
			return items;
		},
		get loading() {
			return loading;
		},
		refresh,
		create,
		rename,
		remove,
		addTrack,
		removeTrack,
		getTracks
	};
})();
