<script lang="ts">
	import { onMount } from 'svelte';
	import { library, type TrackMeta } from '$lib/stores/library.svelte';
	import { player, type Track } from '$lib/stores/player.svelte';
	import { playlists } from '$lib/stores/playlists.svelte';

	onMount(() => {
		library.refresh();
		playlists.refresh();
	});

	function formatDuration(secs: number): string {
		const m = Math.floor(secs / 60);
		const s = Math.floor(secs % 60)
			.toString()
			.padStart(2, '0');
		return `${m}:${s}`;
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

	let queuedTracks = $derived(library.tracks.map(toTrack));

	function playTrack(index: number) {
		player.playQueue(queuedTracks, index);
	}

	let openMenuPath = $state<string | null>(null);
	let newPlaylistName = $state('');

	function toggleMenu(path: string) {
		openMenuPath = openMenuPath === path ? null : path;
		newPlaylistName = '';
	}

	function closeMenu() {
		openMenuPath = null;
	}

	async function addToExisting(playlistId: number, path: string) {
		await playlists.addTrack(playlistId, path);
		closeMenu();
	}

	async function addToNew(path: string) {
		const name = newPlaylistName.trim();
		if (!name) return;
		const id = await playlists.create(name);
		await playlists.addTrack(id, path);
		closeMenu();
	}
</script>

{#if library.tracks.length === 0}
	<p class="hint">
		Point the library scanner at a folder to populate this view — head to
		<strong>Settings</strong> to pick one.
	</p>
{:else}
	<table class="track-list">
		<thead>
			<tr>
				<th>Title</th>
				<th>Artist</th>
				<th>Album</th>
				<th class="dur">Time</th>
				<th class="add-col"></th>
			</tr>
		</thead>
		<tbody>
			{#each library.tracks as t, i (t.path)}
				<tr onclick={() => playTrack(i)}>
					<td>{t.title}</td>
					<td>{t.artist}</td>
					<td>{t.album}</td>
					<td class="dur">{formatDuration(t.duration_secs)}</td>
					<td class="add-col" onclick={(e) => e.stopPropagation()}>
						<div class="add-wrap">
							<button
								class="add-btn"
								onclick={() => toggleMenu(t.path)}
								aria-label="Add to playlist"
							>
								+
							</button>
							{#if openMenuPath === t.path}
								<button class="backdrop" onclick={closeMenu} aria-label="Close menu"></button>
								<div class="menu">
									{#each playlists.items as p (p.id)}
										<button class="menu-item" onclick={() => addToExisting(p.id, t.path)}>
											{p.name}
										</button>
									{/each}
									{#if playlists.items.length > 0}
										<div class="menu-sep"></div>
									{/if}
									<div class="menu-new">
										<input
											type="text"
											placeholder="New playlist…"
											bind:value={newPlaylistName}
											onkeydown={(e) => {
												if (e.key === 'Enter') addToNew(t.path);
											}}
										/>
										<button class="menu-item" onclick={() => addToNew(t.path)}>
											Create &amp; add
										</button>
									</div>
								</div>
							{/if}
						</div>
					</td>
				</tr>
			{/each}
		</tbody>
	</table>
{/if}

<style>
	.hint {
		color: var(--text-muted);
		font-size: 13px;
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
	.add-col {
		width: 36px;
	}
	.add-wrap {
		position: relative;
		display: flex;
		justify-content: center;
	}
	.add-btn {
		width: 22px;
		height: 22px;
		border-radius: 50%;
		color: var(--text-muted);
		font-size: 14px;
		line-height: 1;
		display: flex;
		align-items: center;
		justify-content: center;
	}
	.add-btn:hover {
		color: var(--accent-glow);
		background: var(--bg-elevated-hover);
	}
	.backdrop {
		position: fixed;
		inset: 0;
		z-index: 10;
		background: transparent;
		border: none;
		cursor: default;
	}
	.menu {
		position: absolute;
		top: 100%;
		right: 0;
		z-index: 11;
		margin-top: 4px;
		background: var(--bg-elevated);
		border: 1px solid var(--border-subtle);
		border-radius: var(--radius-sm);
		padding: 6px;
		width: 180px;
		display: flex;
		flex-direction: column;
		gap: 2px;
		box-shadow: 0 8px 24px rgba(0, 0, 0, 0.4);
	}
	.menu-item {
		text-align: left;
		padding: 6px 8px;
		border-radius: var(--radius-sm);
		color: var(--text-secondary);
		font-size: 12px;
		white-space: nowrap;
		overflow: hidden;
		text-overflow: ellipsis;
	}
	.menu-item:hover {
		background: var(--bg-elevated-hover);
		color: var(--text-primary);
	}
	.menu-sep {
		height: 1px;
		background: var(--border-subtle);
		margin: 4px 0;
	}
	.menu-new {
		display: flex;
		flex-direction: column;
		gap: 4px;
	}
	.menu-new input {
		background: var(--bg-void);
		border: 1px solid var(--border-subtle);
		border-radius: var(--radius-sm);
		color: var(--text-primary);
		font-size: 12px;
		padding: 6px 8px;
		font-family: inherit;
	}
	.menu-new input:focus {
		outline: 1px solid var(--accent-glow);
		outline-offset: -1px;
	}
</style>
