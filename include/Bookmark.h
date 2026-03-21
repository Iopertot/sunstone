#pragma once
#include <QString>
#include <QDateTime>

struct Bookmark {
    int       id          = -1;
    QString   audiobookId;
    qint64    positionMs  = 0;
    QString   note;
    QDateTime createdAt;

    QString positionString() const {
        qint64 secs = positionMs / 1000;
        int h = secs / 3600;
        int m = (secs % 3600) / 60;
        int s = secs % 60;
        if (h > 0)
            return QString("%1:%2:%3")
                .arg(h).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
        return QString("%1:%2").arg(m).arg(s, 2, 10, QChar('0'));
    }
};
