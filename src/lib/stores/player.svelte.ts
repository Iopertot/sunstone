import { invoke } from '@tauri-apps/api/core';

export interface Track {
	id: string;
	path: string;
	title: string;
	artist: string;
	album: string;
	durationSecs: number;
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

export const player = (() => {
	let current = $state<Track | null>(null);
	let isPlaying = $state(false);
	let currentTimeSecs = $state(0);

	function load(track: Track) {
		const g = getGraph();
		current = track;
		g.element.src = convertFileSrc(track.path);
		void g.element.play().then(() => {
			isPlaying = true;
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
		});
	}

	function toggle() {
		const g = getGraph();
		if (g.element.paused) {
			void g.element.play();
			isPlaying = true;
		} else {
			g.element.pause();
			isPlaying = false;
		}
	}

	function seek(secs: number) {
		getGraph().element.currentTime = secs;
	}

	function setVolume(v: number) {
		getGraph().gainNode.gain.value = v;
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
		get analyser() {
			return getGraph().analyserNode;
		},
		load,
		toggle,
		seek,
		setVolume
	};
})();

function convertFileSrc(path: string): string {
	return `asset://localhost/${encodeURIComponent(path)}`;
}
