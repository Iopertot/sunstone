<script lang="ts">
	import { onMount } from 'svelte';
	import { library, type TrackMeta } from '$lib/stores/library.svelte';
	import { player } from '$lib/stores/player.svelte';

	onMount(() => {
		library.refresh();
	});

	function formatDuration(secs: number): string {
		const m = Math.floor(secs / 60);
		const s = Math.floor(secs % 60)
			.toString()
			.padStart(2, '0');
		return `${m}:${s}`;
	}

	function playTrack(t: TrackMeta) {
		player.load({
			id: t.path,
			path: t.path,
			title: t.title,
			artist: t.artist,
			album: t.album,
			durationSecs: t.duration_secs
		});
	}
</script>

{#if library.tracks.length === 0}
	<p class="hint">
		Point the library scanner at a folder to populate this view — head to
		<strong>Settings</strong> to pick one, or see
		<code>src-tauri/src/library.rs</code>.
	</p>
{:else}
	<table class="track-list">
		<thead>
			<tr>
				<th>Title</th>
				<th>Artist</th>
				<th>Album</th>
				<th class="dur">Time</th>
			</tr>
		</thead>
		<tbody>
			{#each library.tracks as t (t.path)}
				<tr onclick={() => playTrack(t)}>
					<td>{t.title}</td>
					<td>{t.artist}</td>
					<td>{t.album}</td>
					<td class="dur">{formatDuration(t.duration_secs)}</td>
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
	code {
		color: var(--accent-glow);
		font-family: var(--font-mono);
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
</style>
