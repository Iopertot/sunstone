#pragma once
#include "Audiobook.h"
#include <QListWidget>

class ChapterList : public QListWidget {
    Q_OBJECT
public:
    explicit ChapterList(QWidget* parent = nullptr);
    void setChapters(const QVector<Chapter>& chapters);
    void highlightCurrent(qint64 positionMs);

signals:
    void chapterSelected(qint64 startMs);

private:
    QVector<Chapter> m_chapters;
};
