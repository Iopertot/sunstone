#include "AudioPlayer.h"
#include <QUrl>

AudioPlayer::AudioPlayer(QObject* parent)
    : QObject(parent)
    , m_player(new QMediaPlayer(this))
    , m_audio(new QAudioOutput(this))
{
    m_player->setAudioOutput(m_audio);
    m_audio->setVolume(0.8f);

    connect(m_player, &QMediaPlayer::positionChanged,
            this, &AudioPlayer::positionChanged);
    connect(m_player, &QMediaPlayer::durationChanged,
            this, &AudioPlayer::durationChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged,
            this, [this](QMediaPlayer::PlaybackState s) {
        emit playbackStateChanged(s == QMediaPlayer::PlayingState);
    });
    connect(m_player, &QMediaPlayer::errorOccurred,
            this, [this](QMediaPlayer::Error, const QString& str) {
        emit mediaError(str);
    });
}

void AudioPlayer::setSource(const QString& filePath) {
    m_player->setSource(QUrl::fromLocalFile(filePath));
}

void AudioPlayer::play()  { m_player->play();  }
void AudioPlayer::pause() { m_player->pause(); }
void AudioPlayer::stop()  { m_player->stop();  }

void AudioPlayer::seekTo(qint64 ms) { m_player->setPosition(ms); }

void AudioPlayer::setVolume(float v) { m_audio->setVolume(v); }

void AudioPlayer::setPlaybackRate(float rate) {
    m_player->setPlaybackRate(static_cast<qreal>(rate));
}

bool  AudioPlayer::isPlaying()    const { return m_player->playbackState() == QMediaPlayer::PlayingState; }
qint64 AudioPlayer::positionMs()  const { return m_player->position(); }
qint64 AudioPlayer::durationMs()  const { return m_player->duration(); }
float  AudioPlayer::volume()      const { return m_audio->volume(); }
float  AudioPlayer::playbackRate()const { return static_cast<float>(m_player->playbackRate()); }
