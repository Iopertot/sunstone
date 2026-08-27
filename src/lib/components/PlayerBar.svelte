<script lang="ts">
	import { player } from '$lib/stores/player.svelte';

	let progressPct = $derived(
		player.current ? (player.currentTimeSecs / player.current.durationSecs) * 100 : 0
	);
	let volumePct = $derived(Math.round(player.volume * 100));

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
			{#if player.artDataUrl}
				<img class="art" src={player.artDataUrl} alt="" />
			{:else}
				<div class="art-placeholder"></div>
			{/if}
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
			<button
				aria-label="Previous"
				onclick={() => player.previous()}
				disabled={!player.current}
			>
				⏮
			</button>
			<button class="play-toggle" onclick={() => player.toggle()} aria-label="Play/Pause">
				{player.isPlaying ? '⏸' : '▶'}
			</button>
			<button aria-label="Next" onclick={() => player.next()} disabled={!player.hasNext}>
				⏭
			</button>
			<button
				class="repeat-toggle"
				class:active={player.repeatMode !== 'off'}
				onclick={() => player.cycleRepeat()}
				aria-label="Repeat mode: {player.repeatMode}"
			>
				{player.repeatMode === 'one' ? '🔂' : '🔁'}
			</button>
		</div>
		<div class="scrub">
			<span class="time">{formatTime(player.currentTimeSecs)}</span>
			<input
				type="range"
				class="scrub-bar"
				min="0"
				max={player.current?.durationSecs ?? 0}
				step="0.1"
				value={player.currentTimeSecs}
				oninput={(event) => player.seek(Number(event.currentTarget.value))}
				disabled={!player.current}
				style:background="linear-gradient(to right, var(--accent-copper) {progressPct}%, var(--border-subtle) {progressPct}%)"
				aria-label="Seek"
			/>
			<span class="time">{player.current ? formatTime(player.current.durationSecs) : '0:00'}</span>
		</div>
	</div>

	<div class="volume-control">
		<input
			type="range"
			min="0"
			max="1"
			step="0.01"
			value={player.volume}
			oninput={(event) => player.setVolume(Number(event.currentTarget.value))}
			style:background="linear-gradient(to right, var(--accent-copper) {volumePct}%, var(--border-subtle) {volumePct}%)"
			aria-label="Volume"
		/>
	</div>
</div>

<style>
	.player-bar {
		height: var(--player-bar-height);
		background: var(--bg-elevated);
		border-top: 1px solid var(--border-subtle);
		display: grid;
		grid-template-columns: 240px 1fr 130px;
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
	.art-placeholder,
	.art {
		width: 48px;
		height: 48px;
		border-radius: var(--radius-sm);
		flex-shrink: 0;
	}
	.art-placeholder {
		background: var(--accent-gradient);
	}
	.art {
		object-fit: cover;
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
	.controls button:disabled {
		color: var(--text-muted);
		opacity: 0.4;
		cursor: default;
	}
	.controls button:disabled:hover {
		color: var(--text-muted);
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
	.repeat-toggle {
		font-size: 12px;
		opacity: 0.5;
	}
	.repeat-toggle.active {
		opacity: 1;
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
	.scrub-bar {
		flex: 1;
		appearance: none;
		-webkit-appearance: none;
		height: 4px;
		border-radius: 2px;
		outline: none;
		cursor: pointer;
	}
	.scrub-bar:disabled {
		cursor: default;
	}
	.scrub-bar::-webkit-slider-thumb {
		appearance: none;
		-webkit-appearance: none;
		width: 10px;
		height: 10px;
		border-radius: 50%;
		background: var(--accent-glow);
		cursor: pointer;
	}
	.scrub-bar::-moz-range-thumb {
		width: 10px;
		height: 10px;
		border: none;
		border-radius: 50%;
		background: var(--accent-glow);
		cursor: pointer;
	}
	.scrub-bar::-moz-range-track {
		height: 4px;
		border-radius: 2px;
		background: transparent;
	}

	.volume-control {
		display: flex;
		align-items: center;
		justify-self: end;
		width: 130px;
	}
	.volume-control input[type='range'] {
		appearance: none;
		-webkit-appearance: none;
		width: 100%;
		height: 4px;
		border-radius: 2px;
		outline: none;
		cursor: pointer;
	}
	.volume-control input[type='range']::-webkit-slider-thumb {
		appearance: none;
		-webkit-appearance: none;
		width: 10px;
		height: 10px;
		border-radius: 50%;
		background: var(--accent-glow);
		cursor: pointer;
	}
	.volume-control input[type='range']::-moz-range-thumb {
		width: 10px;
		height: 10px;
		border: none;
		border-radius: 50%;
		background: var(--accent-glow);
		cursor: pointer;
	}
	.volume-control input[type='range']::-moz-range-track {
		height: 4px;
		border-radius: 2px;
		background: transparent;
	}
</style>