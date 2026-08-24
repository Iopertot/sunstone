<script lang="ts">
	import PlayerBar from '$lib/components/PlayerBar.svelte';
	import LibraryView from '$lib/components/views/LibraryView.svelte';
	import PlaylistsView from '$lib/components/views/PlaylistsView.svelte';
	import NowScrobblingView from '$lib/components/views/NowScrobblingView.svelte';
	import SettingsView from '$lib/components/views/SettingsView.svelte';

	type ViewId = 'library' | 'playlists' | 'now-scrobbling' | 'settings';

	const NAV_ITEMS: { id: ViewId; label: string }[] = [
		{ id: 'library', label: 'Library' },
		{ id: 'playlists', label: 'Playlists' },
		{ id: 'now-scrobbling', label: 'Now Scrobbling' },
		{ id: 'settings', label: 'Settings' }
	];

	let activeView = $state<ViewId>('library');
</script>

<div class="app-shell">
	<aside class="sidebar">
		<div class="brand">Sunstone</div>
		<nav>
			{#each NAV_ITEMS as item (item.id)}
				<button
					class="nav-item"
					class:active={activeView === item.id}
					onclick={() => (activeView = item.id)}
				>
					{item.label}
				</button>
			{/each}
		</nav>
	</aside>

	<main class="content">
		{#if activeView === 'library'}
			<LibraryView />
		{:else if activeView === 'playlists'}
			<PlaylistsView />
		{:else if activeView === 'now-scrobbling'}
			<NowScrobblingView />
		{:else if activeView === 'settings'}
			<SettingsView />
		{/if}
	</main>

	<PlayerBar />
</div>

<style>
	.app-shell {
		display: grid;
		grid-template-columns: var(--sidebar-width) 1fr;
		grid-template-rows: 1fr var(--player-bar-height);
		height: 100vh;
	}

	.sidebar {
		grid-row: 1 / 2;
		background: var(--bg-surface);
		border-right: 1px solid var(--border-subtle);
		padding: 20px 12px;
		display: flex;
		flex-direction: column;
		gap: 20px;
	}
	.brand {
		font-weight: 700;
		font-size: 15px;
		letter-spacing: 0.02em;
		color: var(--text-primary);
		padding-left: 8px;
		background: var(--accent-gradient);
		background-clip: text;
		-webkit-background-clip: text;
		-webkit-text-fill-color: transparent;
	}
	nav {
		display: flex;
		flex-direction: column;
		gap: 2px;
	}
	.nav-item {
		text-align: left;
		padding: 8px 10px;
		border-radius: var(--radius-sm);
		color: var(--text-secondary);
		font-size: 13px;
	}
	.nav-item:hover {
		background: var(--bg-elevated-hover);
		color: var(--text-primary);
	}
	.nav-item.active {
		background: var(--bg-elevated);
		color: var(--text-primary);
	}

	.content {
		grid-row: 1 / 2;
		background: var(--bg-void);
		padding: 24px;
		overflow-y: auto;
	}

	:global(.player-bar) {
		grid-column: 1 / 3;
		grid-row: 2 / 3;
	}
</style>
