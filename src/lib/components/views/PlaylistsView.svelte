<script lang="ts">
	import { onMount } from 'svelte';
	import { playlists } from '$lib/stores/playlists.svelte';
	import { player, type Track } from '$lib/stores/player.svelte';
	import type { TrackMeta } from '$lib/stores/library.svelte';

	onMount(() => {
		playlists.refresh();
	});

	let selectedId = $state<number | null>(null);
	let selectedTracks = $state<TrackMeta[]>([]);
	let loadingTracks = $state(false);
	let newPlaylistName = $state('');
	let renameValue = $state('');
	let renaming = $state(false);

	let selectedSummary = $derived(playlists.items.find((p) => p.id === selectedId) ?? null);

	async function openPlaylist(id: number) {
		selectedId = id;
		renaming = false;
		loadingTracks = true;
		try {
			selectedTracks = await playlists.getTracks(id);
		} finally {
			loadingTracks = false;
		}
	}

	function closePlaylist() {
		selectedId = null;
		selectedTracks = [];
		renaming = false;
	}

	async function handleCreate() {
		const name = newPlaylistName.trim();
		if (!name) return;
		newPlaylistName = '';
		await playlists.create(name);
	}

	async function handleDelete(id: number, event: MouseEvent) {
		event.stopPropagation();
		await playlists.remove(id);
		if (selectedId === id) closePlaylist();
	}

	function startRename() {
		if (!selectedSummary) return;
		renameValue = selectedSummary.name;
		renaming = true;
	}

	async function commitRename() {
		if (!renaming || selectedId === null) return;
		renaming = false;
		const name = renameValue.trim();
		if (!name) return;
		await playlists.rename(selectedId, name);
	}

	function toTrack(t: TrackMeta): Track {
		return {
			id: t.path,
			path: t.path,
			title: t.title,
			artist: t.artist,
			album: t.album,
			durationSecs: t.duration_secs
		};
	}

	function playAll() {
		if (selectedTracks.length === 0) return;
		player.playQueue(selectedTracks.map(toTrack), 0);
	}

	function playFrom(index: number) {
		player.playQueue(selectedTracks.map(toTrack), index);
	}

	async function removeTrack(path: string) {
		if (selectedId === null) return;
		await playlists.removeTrack(selectedId, path);
		selectedTracks = selectedTracks.filter((t) => t.path !== path);
		await playlists.refresh();
	}

	function formatDuration(secs: number): string {
		const m = Math.floor(secs / 60);
		const s = Math.floor(secs % 60)
			.toString()
			.padStart(2, '0');
		return `${m}:${s}`;
	}
</script>

{#if selectedId === null}
	<div class="new-playlist">
		<input
			type="text"
			placeholder="New playlist name…"
			bind:value={newPlaylistName}
			onkeydown={(e) => {
				if (e.key === 'Enter') handleCreate();
			}}
		/>
		<button class="primary" onclick={handleCreate}>Create</button>
	</div>

	{#if playlists.items.length === 0}
		<p class="hint">
			No playlists yet — create one above, or add a track to one from the Library view.
		</p>
	{:else}
		<div class="playlist-list">
			{#each playlists.items as p (p.id)}
				<div class="playlist-row">
					<button class="playlist-main" onclick={() => openPlaylist(p.id)}>
						<span class="playlist-name">{p.name}</span>
						<span class="playlist-count">
							{p.track_count}
							{p.track_count === 1 ? 'track' : 'tracks'}
						</span>
					</button>
					<button
						class="delete-btn"
						onclick={(e) => handleDelete(p.id, e)}
						aria-label="Delete playlist"
					>
						✕
					</button>
				</div>
			{/each}
		</div>
	{/if}
{:else}
	<div class="detail-header">
		<button class="back-btn" onclick={closePlaylist}>← Playlists</button>
		{#if renaming}
			<input
				type="text"
				class="rename-input"
				bind:value={renameValue}
				onkeydown={(e) => {
					if (e.key === 'Enter') commitRename();
					if (e.key === 'Escape') renaming = false;
				}}
				onblur={commitRename}
			/>
		{:else}
			<button class="rename-trigger" onclick={startRename}>{selectedSummary?.name ?? ''}</button>
		{/if}
		<button class="primary" onclick={playAll} disabled={selectedTracks.length === 0}>
			Play all
		</button>
	</div>

	{#if loadingTracks}
		<p class="hint">Loading…</p>
	{:else if selectedTracks.length === 0}
		<p class="hint">This playlist is empty — add tracks to it from the Library view.</p>
	{:else}
		<table class="track-list">
			<thead>
				<tr>
					<th>Title</th>
					<th>Artist</th>
					<th>Album</th>
					<th class="dur">Time</th>
					<th class="remove-col"></th>
				</tr>
			</thead>
			<tbody>
				{#each selectedTracks as t, i (t.path)}
					<tr onclick={() => playFrom(i)}>
						<td>{t.title}</td>
						<td>{t.artist}</td>
						<td>{t.album}</td>
						<td class="dur">{formatDuration(t.duration_secs)}</td>
						<td class="remove-col">
							<button
								class="remove-btn"
								onclick={(e) => {
									e.stopPropagation();
									removeTrack(t.path);
								}}
								aria-label="Remove from playlist"
							>
								✕
							</button>
						</td>
					</tr>
				{/each}
			</tbody>
		</table>
	{/if}
{/if}

<style>
	.hint {
		color: var(--text-muted);
		font-size: 13px;
	}
	.new-playlist {
		display: flex;
		gap: 8px;
		margin-bottom: 20px;
		max-width: 420px;
	}
	.new-playlist input {
		flex: 1;
		background: var(--bg-void);
		border: 1px solid var(--border-subtle);
		border-radius: var(--radius-sm);
		color: var(--text-primary);
		font-size: 13px;
		padding: 8px 10px;
		font-family: inherit;
	}
	.new-playlist input:focus {
		outline: 1px solid var(--accent-glow);
		outline-offset: -1px;
	}
	.primary {
		background: var(--accent-gradient);
		color: var(--bg-void);
		padding: 8px 16px;
		border-radius: var(--radius-sm);
		font-weight: 600;
		font-size: 13px;
	}
	.primary:disabled {
		opacity: 0.5;
		cursor: default;
	}
	.playlist-list {
		display: flex;
		flex-direction: column;
		gap: 2px;
		max-width: 420px;
	}
	.playlist-row {
		display: flex;
		align-items: center;
		justify-content: space-between;
		border-radius: var(--radius-sm);
	}
	.playlist-row:hover {
		background: var(--bg-elevated-hover);
	}
	.playlist-main {
		flex: 1;
		display: flex;
		flex-direction: column;
		align-items: flex-start;
		min-width: 0;
		text-align: left;
		padding: 10px 12px;
	}
	.playlist-name {
		color: var(--text-primary);
		font-size: 13px;
		font-weight: 600;
	}
	.playlist-count {
		color: var(--text-muted);
		font-size: 11px;
	}
	.delete-btn {
		color: var(--text-muted);
		font-size: 12px;
		width: 24px;
		height: 24px;
		border-radius: 50%;
		display: flex;
		align-items: center;
		justify-content: center;
		flex-shrink: 0;
		margin-right: 8px;
	}
	.delete-btn:hover {
		color: var(--color-danger);
		background: var(--bg-elevated-hover);
	}

	.detail-header {
		display: flex;
		align-items: center;
		gap: 16px;
		margin-bottom: 20px;
	}
	.back-btn {
		color: var(--text-muted);
		font-size: 12px;
		flex-shrink: 0;
	}
	.back-btn:hover {
		color: var(--accent-glow);
	}
	.rename-trigger {
		flex: 1;
		font-size: 15px;
		color: var(--text-primary);
		margin: 0;
		cursor: text;
		min-width: 0;
		white-space: nowrap;
		overflow: hidden;
		text-overflow: ellipsis;
		text-align: left;
		background: none;
		font-weight: 600;
	}
	.rename-input {
		flex: 1;
		background: var(--bg-void);
		border: 1px solid var(--border-subtle);
		border-radius: var(--radius-sm);
		color: var(--text-primary);
		font-size: 15px;
		padding: 4px 8px;
		font-family: inherit;
		min-width: 0;
	}
	.rename-input:focus {
		outline: 1px solid var(--accent-glow);
		outline-offset: -1px;
	}

	.track-list {
		width: 100%;
		border-collapse: collapse;
		font-size: 13px;
	}
	.track-list th {
		text-align: left;
		color: var(--text-muted);
		font-weight: 500;
		padding: 6px 10px;
		border-bottom: 1px solid var(--border-subtle);
	}
	.track-list td {
		padding: 8px 10px;
		color: var(--text-secondary);
		border-bottom: 1px solid var(--border-subtle);
	}
	.track-list tr:hover td {
		background: var(--bg-elevated-hover);
		color: var(--text-primary);
		cursor: pointer;
	}
	.dur {
		text-align: right;
		font-family: var(--font-mono);
		color: var(--text-muted);
		width: 60px;
	}
	.remove-col {
		width: 32px;
	}
	.remove-btn {
		color: var(--text-muted);
		font-size: 11px;
		width: 20px;
		height: 20px;
		border-radius: 50%;
		display: flex;
		align-items: center;
		justify-content: center;
	}
	.remove-btn:hover {
		color: var(--color-danger);
		background: var(--bg-elevated-hover);
	}
</style>
