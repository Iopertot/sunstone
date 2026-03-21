#pragma once
#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QString>

class AudioPlayer : public QObject {
    Q_OBJECT
public:
    explicit AudioPlayer(QObject* parent = nullptr);

    void    setSource(const QString& filePath);
    void    play();
    void    pause();
    void    stop();
    void    seekTo(qint64 ms);
    void    setVolume(float v);           // 0.0–1.0
    void    setPlaybackRate(float rate);  // 0.5–3.0
    bool    isPlaying() const;
    qint64  positionMs() const;
    qint64  durationMs() const;
    float   volume() const;
    float   playbackRate() const;

signals:
    void positionChanged(qint64 ms);
    void durationChanged(qint64 ms);
    void playbackStateChanged(bool playing);
    void mediaError(const QString& msg);

private:
    QMediaPlayer* m_player;
    QAudioOutput* m_audio;
};
