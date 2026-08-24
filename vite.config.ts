import { sveltekit } from '@sveltejs/kit/vite';
import { defineConfig } from 'vite';

// Tauri expects a fixed port and to be able to reach the dev server from
// the webview. See https://v2.tauri.app/start/frontend/sveltekit/
export default defineConfig(async () => ({
	plugins: [sveltekit()],
	clearScreen: false,
	server: {
		port: 1420,
		strictPort: true,
		watch: {
			// don't watch the Rust backend, Tauri's own CLI handles that
			ignored: ['**/src-tauri/**']
		}
	}
}));
