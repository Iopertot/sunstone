<script lang="ts">
	import { player } from '$lib/stores/player.svelte';

	let progressPct = $derived(
		player.current ? (player.currentTimeSecs / player.current.durationSecs) * 100 : 0
	);

	function formatTime(s: number): string {
		const m = Math.floor(s / 60);
		const sec = Math.floor(s % 60)
			.toString()
			.padStart(2, '0');
		return `${m}:${sec}`;
	}
</script>

<div class="player-bar">
	<div class="now-playing">
		{#if player.current}
			<div class="art-placeholder"></div>
			<div class="meta">
				<span class="title">{player.current.title}</span>
				<span class="artist">{player.current.artist}</span>
			</div>
		{:else}
			<span class="empty">Nothing playing</span>
		{/if}
	</div>

	<div class="transport">
		<div class="controls">
			<button aria-label="Previous">⏮</button>
			<button class="play-toggle" onclick={() => player.toggle()} aria-label="Play/Pause">
				{player.isPlaying ? '⏸' : '▶'}
			</button>
			<button aria-label="Next">⏭</button>
		</div>
		<div class="scrub">
			<span class="time">{formatTime(player.currentTimeSecs)}</span>
			<div class="track">
				<div class="fill" style:width="{progressPct}%"></div>
			</div>
			<span class="time">{player.current ? formatTime(player.current.durationSecs) : '0:00'}</span>
		</div>
	</div>
</div>

<style>
	.player-bar {
		height: var(--player-bar-height);
		background: var(--bg-elevated);
		border-top: 1px solid var(--border-subtle);
		display: grid;
		grid-template-columns: 240px 1fr;
		align-items: center;
		padding: 0 16px;
		gap: 24px;
	}

	.now-playing {
		display: flex;
		align-items: center;
		gap: 12px;
		min-width: 0;
	}
	.art-placeholder {
		width: 48px;
		height: 48px;
		border-radius: var(--radius-sm);
		background: var(--accent-gradient);
		flex-shrink: 0;
	}
	.meta {
		display: flex;
		flex-direction: column;
		min-width: 0;
	}
	.title {
		color: var(--text-primary);
		font-weight: 600;
		white-space: nowrap;
		overflow: hidden;
		text-overflow: ellipsis;
	}
	.artist {
		color: var(--text-muted);
		font-size: 12px;
	}
	.empty {
		color: var(--text-muted);
	}

	.transport {
		display: flex;
		flex-direction: column;
		gap: 6px;
		align-items: center;
	}
	.controls {
		display: flex;
		gap: 18px;
		align-items: center;
	}
	.controls button {
		color: var(--text-secondary);
		font-size: 14px;
	}
	.controls button:hover {
		color: var(--accent-glow);
	}
	.play-toggle {
		width: 32px;
		height: 32px;
		border-radius: 50%;
		background: var(--accent-gradient);
		color: var(--bg-void);
		display: flex;
		align-items: center;
		justify-content: center;
	}

	.scrub {
		display: flex;
		align-items: center;
		gap: 8px;
		width: 100%;
		max-width: 520px;
	}
	.time {
		color: var(--text-muted);
		font-family: var(--font-mono);
		font-size: 11px;
		width: 34px;
	}
	.track {
		flex: 1;
		height: 4px;
		border-radius: 2px;
		background: var(--border-subtle);
		overflow: hidden;
	}
	.fill {
		height: 100%;
		background: var(--accent-gradient);
	}
</style>
