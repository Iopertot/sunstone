#pragma once
#include <QWidget>

class WaveformWidget : public QWidget {
    Q_OBJECT
public:
    explicit WaveformWidget(QWidget* parent = nullptr);

    void setDuration(qint64 durationMs);
    void setPosition(qint64 posMs);
    void setBookmarks(const QList<qint64>& positionsMs);

signals:
    void seekRequested(qint64 ms);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;

private:
    qint64       m_duration = 1;
    qint64       m_position = 0;
    QList<qint64> m_bookmarks;
    bool          m_dragging = false;

    qint64 xToMs(int x) const;
};
