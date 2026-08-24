<script lang="ts">
	import { onMount } from 'svelte';
	import { lastfm } from '$lib/stores/lastfm.svelte';
	import { player } from '$lib/stores/player.svelte';

	onMount(() => {
		lastfm.refreshStatus();
	});
</script>

<div class="scrobble-status">
	{#if !lastfm.checked}
		<p class="hint">Checking Last.fm connection…</p>
	{:else if !lastfm.connected}
		<p class="hint">
			Not connected to Last.fm yet — head to <strong>Settings</strong> to connect your account.
		</p>
	{:else if player.current}
		<div class="live">
			<span class="dot is-live"></span>
			<div>
				<div class="title">{player.current.title}</div>
				<div class="artist">{player.current.artist}</div>
			</div>
		</div>
		<p class="hint">
			Scrobbles once it's played half its length (or 4 minutes, whichever comes first) — see
			<code>src-tauri/src/lastfm.rs</code> for the full rules.
		</p>
	{:else}
		<p class="hint">Connected to Last.fm. Nothing playing right now.</p>
	{/if}
</div>

<style>
	.hint {
		color: var(--text-muted);
		font-size: 13px;
	}
	code {
		color: var(--accent-glow);
		font-family: var(--font-mono);
	}
	.live {
		display: flex;
		align-items: center;
		gap: 10px;
		margin-bottom: 12px;
	}
	.dot {
		width: 10px;
		height: 10px;
		border-radius: 50%;
		flex-shrink: 0;
	}
	.title {
		color: var(--text-primary);
		font-weight: 600;
	}
	.artist {
		color: var(--text-secondary);
		font-size: 12px;
	}
</style>
