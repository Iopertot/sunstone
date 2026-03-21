#include "WaveformWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <cmath>

WaveformWidget::WaveformWidget(QWidget* parent) : QWidget(parent) {
    setFixedHeight(56);
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);
}

void WaveformWidget::setDuration(qint64 d) { m_duration = d > 0 ? d : 1; update(); }
void WaveformWidget::setPosition(qint64 p) { m_position = p; update(); }
void WaveformWidget::setBookmarks(const QList<qint64>& bm) { m_bookmarks = bm; update(); }

qint64 WaveformWidget::xToMs(int x) const {
    return static_cast<qint64>(static_cast<double>(x) / width() * m_duration);
}

void WaveformWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int W = width(), H = height();
    double progress = static_cast<double>(m_position) / m_duration;
    int fillX = static_cast<int>(progress * W);

    // Background track
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#1e1c18"));
    p.drawRoundedRect(0, H/2 - 3, W, 6, 3, 3);

    // Decorative bars (waveform look)
    int bars = W / 4;
    for (int i = 0; i < bars; ++i) {
        float t  = static_cast<float>(i) / bars;
        float h  = 6 + 18 * std::abs(std::sin(t * 37.0f + 0.5f) * std::cos(t * 19.0f));
        int bx   = i * 4;
        int bh   = static_cast<int>(h);
        int by   = H/2 - bh/2;

        bool played = bx <= fillX;
        QColor col = played ? QColor("#c9a96e") : QColor("#3a3830");
        col.setAlphaF(played ? 0.85f : 0.5f);
        p.setBrush(col);
        p.drawRoundedRect(bx + 1, by, 2, bh, 1, 1);
    }

    // Playhead
    p.setPen(QPen(QColor("#f0c27f"), 2));
    p.drawLine(fillX, 4, fillX, H - 4);

    // Bookmark markers
    for (qint64 bms : m_bookmarks) {
        int bx = static_cast<int>(static_cast<double>(bms) / m_duration * W);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#e05050"));
        p.drawEllipse(bx - 4, 2, 8, 8);
    }
}

void WaveformWidget::mousePressEvent(QMouseEvent* e) {
    m_dragging = true;
    emit seekRequested(xToMs(e->pos().x()));
}

void WaveformWidget::mouseMoveEvent(QMouseEvent* e) {
    if (m_dragging && (e->buttons() & Qt::LeftButton))
        emit seekRequested(xToMs(e->pos().x()));
}
