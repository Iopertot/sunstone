#pragma once
#include <QString>
#include <QDateTime>
#include <QPixmap>
#include <QVector>

struct Chapter {
    QString title;
    qint64  startMs;   // milliseconds from start
    qint64  endMs;
};

struct Audiobook {
    QString   id;           // SHA1 of file path
    QString   filePath;
    QString   title;
    QString   author;
    QString   narrator;
    QString   series;
    int       seriesIndex   = 0;
    qint64    durationMs    = 0;
    int       year          = 0;
    QString   description;
    QPixmap   coverArt;
    QByteArray coverArtData; // raw JPEG/PNG bytes
    QVector<Chapter> chapters;
    qint64    lastPositionMs = 0;
    QDateTime lastPlayed;
    bool      isFinished    = false;
    float     playbackSpeed = 1.0f;

    // Computed helpers
    QString durationString() const {
        qint64 secs = durationMs / 1000;
        int h = secs / 3600;
        int m = (secs % 3600) / 60;
        int s = secs % 60;
        if (h > 0)
            return QString("%1h %2m").arg(h).arg(m, 2, 10, QChar('0'));
        return QString("%1m %2s").arg(m).arg(s, 2, 10, QChar('0'));
    }

    int progressPercent() const {
        if (durationMs <= 0) return 0;
        return static_cast<int>(lastPositionMs * 100 / durationMs);
    }
};
