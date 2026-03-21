#include "ChapterList.h"
#include <QListWidgetItem>

ChapterList::ChapterList(QWidget* parent) : QListWidget(parent) {
    setStyleSheet(R"(
        QListWidget {
            background: #1a1814;
            border: none;
            color: #c8c0b0;
            font-size: 12px;
        }
        QListWidget::item {
            padding: 8px 12px;
            border-bottom: 1px solid #2a2820;
        }
        QListWidget::item:selected {
            background: #2d2820;
            color: #c9a96e;
        }
        QListWidget::item:hover {
            background: #252218;
        }
    )");
    connect(this, &QListWidget::itemDoubleClicked,
            this, [this](QListWidgetItem* item) {
        int row = this->row(item);
        if (row >= 0 && row < m_chapters.size())
            emit chapterSelected(m_chapters[row].startMs);
    });
}

void ChapterList::setChapters(const QVector<Chapter>& chapters) {
    m_chapters = chapters;
    clear();
    for (const auto& ch : chapters) {
        qint64 secs = ch.startMs / 1000;
        int h = secs / 3600, m = (secs % 3600)/60, s = secs % 60;
        QString ts = h > 0
            ? QString("%1:%2:%3").arg(h).arg(m,2,10,QChar('0')).arg(s,2,10,QChar('0'))
            : QString("%1:%2").arg(m).arg(s,2,10,QChar('0'));
        addItem(QString("%1   %2").arg(ts).arg(ch.title));
    }
}

void ChapterList::highlightCurrent(qint64 positionMs) {
    for (int i = m_chapters.size()-1; i >= 0; --i) {
        if (positionMs >= m_chapters[i].startMs) {
            blockSignals(true);
            setCurrentRow(i);
            blockSignals(false);
            return;
        }
    }
}
