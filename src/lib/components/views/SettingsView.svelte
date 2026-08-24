<script lang="ts">
	import { onMount } from 'svelte';
	import { open as openDialog } from '@tauri-apps/plugin-dialog';
	import { lastfm } from '$lib/stores/lastfm.svelte';
	import { library } from '$lib/stores/library.svelte';

	let folderPath = $state<string | null>(null);

	onMount(() => {
		lastfm.refreshStatus();
	});

	async function pickFolder() {
		const selected = await openDialog({ directory: true, multiple: false });
		if (typeof selected === 'string') {
			folderPath = selected;
			await library.scan(selected);
		}
	}
</script>

<div class="settings">
	<section>
		<h2>Music library</h2>
		<p class="hint">Pick a folder to scan for tracks.</p>
		<button class="primary" onclick={pickFolder} disabled={library.scanning}>
			{library.scanning ? 'Scanning…' : 'Choose folder'}
		</button>
		{#if folderPath}
			<p class="hint">
				Last scanned <code>{folderPath}</code> — {library.tracks.length} tracks found.
			</p>
		{/if}
		{#if library.lastScanError}
			<p class="error">{library.lastScanError}</p>
		{/if}
	</section>

	<section>
		<h2>Last.fm</h2>
		{#if !lastfm.checked}
			<p class="hint">Checking connection…</p>
		{:else if lastfm.connected}
			<p class="hint">✓ Connected.</p>
		{:else if lastfm.pendingToken}
			<p class="hint">Approve access in the browser tab that just opened, then confirm below.</p>
			<button class="primary" onclick={() => lastfm.finishAuth()} disabled={lastfm.busy}>
				{lastfm.busy ? 'Confirming…' : "I've approved it"}
			</button>
		{:else}
			<button class="primary" onclick={() => lastfm.startAuth()} disabled={lastfm.busy}>
				{lastfm.busy ? 'Opening…' : 'Connect Last.fm'}
			</button>
		{/if}
		{#if lastfm.error}
			<p class="error">{lastfm.error}</p>
		{/if}
		<p class="hint small">
			Needs <code>api_key</code>/<code>api_secret</code> in your config file first — see the
			README's Setup section for where that file lives.
		</p>
	</section>
</div>

<style>
	.settings {
		display: flex;
		flex-direction: column;
		gap: 28px;
		max-width: 480px;
	}
	h2 {
		font-size: 14px;
		color: var(--text-primary);
		margin: 0 0 8px;
	}
	.hint {
		color: var(--text-muted);
		font-size: 13px;
		margin: 4px 0;
	}
	.hint.small {
		font-size: 11px;
	}
	.error {
		color: var(--color-danger);
		font-size: 12px;
		margin-top: 6px;
	}
	code {
		color: var(--accent-glow);
		font-family: var(--font-mono);
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
		opacity: 0.6;
		cursor: default;
	}
</style>
