import { convertFileSrc, invoke } from '@tauri-apps/api/core';

export interface Track {
	id: string;
	path: string;
	title: string;
	artist: string;
	album: string;
	durationSecs: number;
}

interface RawAlbumArt {
	mime: string;
	data_base64: string;
}

const artCache = new Map<string, string | null>();

async function fetchAlbumArt(path: string): Promise<string | null> {
	if (artCache.has(path)) return artCache.get(path) ?? null;
	try {
		const art = await invoke<RawAlbumArt | null>('get_album_art', { path });
		const dataUrl = art ? `data:${art.mime};base64,${art.data_base64}` : null;
		artCache.set(path, dataUrl);
		return dataUrl;
	} catch (err) {
		console.error('failed to fetch album art', err);
		artCache.set(path, null);
		return null;
	}
}

class AudioGraph {
	ctx: AudioContext;
	private el: HTMLAudioElement;
	private source: MediaElementAudioSourceNode;
	gainNode: GainNode;
	analyserNode: AnalyserNode;

	constructor() {
		this.ctx = new AudioContext();
		this.el = new Audio();
		this.el.crossOrigin = 'anonymous';

		this.source = this.ctx.createMediaElementSource(this.el);
		this.gainNode = this.ctx.createGain();
		this.gainNode.gain.value = 0.35;
		this.analyserNode = this.ctx.createAnalyser();
		this.analyserNode.fftSize = 2048;

		this.source.connect(this.gainNode);
		this.gainNode.connect(this.analyserNode);
		this.analyserNode.connect(this.ctx.destination);
	}

	get element() {
		return this.el;
	}
}

let graph: AudioGraph | null = null;
function getGraph(): AudioGraph {
	if (!graph) graph = new AudioGraph();
	return graph;
}

let scrobbleTimer: ReturnType<typeof setTimeout> | null = null;

function armScrobble(track: Track, startedAtUnix: number) {
	if (scrobbleTimer) clearTimeout(scrobbleTimer);
	if (track.durationSecs <= 30) return;

	const thresholdSecs = Math.min(track.durationSecs / 2, 240);
	scrobbleTimer = setTimeout(() => {
		invoke('queue_scrobble', {
			track: {
				artist: track.artist,
				title: track.title,
				album: track.album,
				duration_secs: Math.round(track.durationSecs),
				started_at: startedAtUnix
			}
		}).catch((err) => console.error('failed to queue scrobble', err));
	}, thresholdSecs * 1000);
}

export type RepeatMode = 'off' | 'all' | 'one';

export const player = (() => {
	let current = $state<Track | null>(null);
	let isPlaying = $state(false);
	let currentTimeSecs = $state(0);
	let artDataUrl = $state<string | null>(null);
	let volume = $state(0.35);
	let queue = $state<Track[]>([]);
	let queueIndex = $state(-1);
	let repeatMode = $state<RepeatMode>('off');
	let listenersAttached = false;

	function attachListeners(g: AudioGraph) {
		if (listenersAttached) return;
		listenersAttached = true;

		g.element.addEventListener('timeupdate', () => {
			currentTimeSecs = g.element.currentTime;
		});
		g.element.addEventListener('play', () => {
			isPlaying = true;
		});
		g.element.addEventListener('pause', () => {
			isPlaying = false;
		});
		g.element.addEventListener('ended', () => {
			if (repeatMode === 'one') {
				seek(0);
				void g.element.play();
				return;
			}
			if (queueIndex >= 0 && queueIndex < queue.length - 1) {
				playAt(queueIndex + 1);
			} else if (repeatMode === 'all' && queue.length > 0) {
				playAt(0);
			} else {
				isPlaying = false;
				currentTimeSecs = 0;
			}
		});
		g.element.addEventListener('error', () => {
			console.error('audio element error', g.element.error);
			isPlaying = false;
		});
	}

	function loadAndPlay(track: Track) {
		const g = getGraph();
		attachListeners(g);

		current = track;
		currentTimeSecs = 0;
		artDataUrl = null;
		g.element.src = convertFileSrc(track.path);

		if (g.ctx.state === 'suspended') void g.ctx.resume();

		void g.element
			.play()
			.then(() => {
				const startedAt = Math.floor(Date.now() / 1000);
				armScrobble(track, startedAt);
				invoke('update_now_playing', {
					track: {
						artist: track.artist,
						title: track.title,
						album: track.album,
						duration_secs: Math.round(track.durationSecs)
					}
				}).catch((err) => console.error('failed to update now playing', err));
			})
			.catch((err) => {
				console.error('failed to play track', track.path, err);
			});

		void fetchAlbumArt(track.path).then((url) => {
			if (current?.path === track.path) artDataUrl = url;
		});
	}

	function load(track: Track) {
		queue = [track];
		queueIndex = 0;
		loadAndPlay(track);
	}

	function playQueue(tracks: Track[], index: number) {
		if (index < 0 || index >= tracks.length) return;
		queue = tracks;
		queueIndex = index;
		loadAndPlay(tracks[index]);
	}

	function playAt(index: number) {
		if (index < 0 || index >= queue.length) return;
		queueIndex = index;
		loadAndPlay(queue[index]);
	}

	function next() {
		if (queueIndex < queue.length - 1) {
			playAt(queueIndex + 1);
		} else if (repeatMode === 'all' && queue.length > 0) {
			playAt(0);
		}
	}

	function previous() {
		if (currentTimeSecs > 3 || queueIndex <= 0) {
			seek(0);
			return;
		}
		playAt(queueIndex - 1);
	}

	function cycleRepeat() {
		repeatMode = repeatMode === 'off' ? 'all' : repeatMode === 'all' ? 'one' : 'off';
	}

	function toggle() {
		const g = getGraph();
		if (g.element.paused) {
			void g.element.play();
		} else {
			g.element.pause();
		}
	}

	function seek(secs: number) {
		getGraph().element.currentTime = secs;
		currentTimeSecs = secs;
	}

	function setVolume(v: number) {
		const clamped = Math.max(0, Math.min(1, v));
		volume = clamped;
		getGraph().gainNode.gain.value = clamped;
	}

	return {
		get current() {
			return current;
		},
		get isPlaying() {
			return isPlaying;
		},
		get currentTimeSecs() {
			return currentTimeSecs;
		},
		get artDataUrl() {
			return artDataUrl;
		},
		get analyser() {
			return getGraph().analyserNode;
		},
		get volume() {
			return volume;
		},
		get hasNext() {
			if (queueIndex < 0) return false;
			if (queueIndex < queue.length - 1) return true;
			return repeatMode === 'all' && queue.length > 0;
		},
		get hasPrevious() {
			return queueIndex > 0;
		},
		get repeatMode() {
			return repeatMode;
		},
		load,
		playQueue,
		next,
		previous,
		cycleRepeat,
		toggle,
		seek,
		setVolume
	};
})();