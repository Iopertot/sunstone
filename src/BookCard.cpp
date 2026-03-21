#include "BookCard.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QMouseEvent>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QPixmap>
#include <QPainterPath>
#include <QFont>

static QPixmap defaultCover() {
    QPixmap pm(200, 200);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    QLinearGradient g(0, 0, 200, 200);
    g.setColorAt(0, QColor("#2d2b55"));
    g.setColorAt(1, QColor("#11998e"));
    p.fillRect(pm.rect(), g);
    p.setPen(QColor(255,255,255,120));
    p.setFont(QFont("serif", 48));
    p.drawText(pm.rect(), Qt::AlignCenter, "♫");
    return pm;
}

BookCard::BookCard(const Audiobook& book, QWidget* parent)
    : QWidget(parent), m_bookId(book.id)
{
    setupUi(book);
    applyStyle();
    setFixedWidth(200);
}

void BookCard::setupUi(const Audiobook& book) {
    auto* vl = new QVBoxLayout(this);
    vl->setContentsMargins(0, 0, 0, 8);
    vl->setSpacing(6);

    // Cover image
    m_cover = new QLabel(this);
    m_cover->setFixedSize(200, 200);
    m_cover->setAlignment(Qt::AlignCenter);
    m_cover->setScaledContents(false);

    QPixmap px = book.coverArt.isNull() ? defaultCover() : book.coverArt;
    px = px.scaled(200, 200, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    // Rounded corners via mask
    QPixmap rounded(200, 200);
    rounded.fill(Qt::transparent);
    QPainter pr(&rounded);
    pr.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addRoundedRect(0, 0, 200, 200, 12, 12);
    pr.setClipPath(path);
    pr.drawPixmap(0, 0, px);
    m_cover->setPixmap(rounded);

    // Progress strip at bottom of cover (painted in paintEvent)
    m_progress = book.progressPercent();

    // Text labels
    m_title = new QLabel(this);
    m_title->setWordWrap(true);
    m_title->setMaximumWidth(196);
    m_title->setText(book.title);

    m_author = new QLabel(this);
    m_author->setText(book.author);
    m_author->setMaximumWidth(196);

    m_duration = new QLabel(this);
    m_duration->setText(book.durationString());

    vl->addWidget(m_cover);
    vl->addWidget(m_title);
    vl->addWidget(m_author);
    vl->addWidget(m_duration);
}

void BookCard::applyStyle() {
    setStyleSheet(R"(
        BookCard {
            background: transparent;
        }
        QLabel#title {
            color: #e8e4d9;
            font-family: 'Georgia', serif;
            font-size: 13px;
            font-weight: bold;
        }
        QLabel#author {
            color: #a09880;
            font-size: 11px;
        }
        QLabel#duration {
            color: #6b6557;
            font-size: 10px;
        }
    )");
    m_title->setObjectName("title");
    m_author->setObjectName("author");
    m_duration->setObjectName("duration");

    setCursor(Qt::PointingHandCursor);
}

void BookCard::updateBook(const Audiobook& book) {
    m_progress = book.progressPercent();
    m_title->setText(book.title);
    m_author->setText(book.author);
    m_duration->setText(book.durationString());
    update();
}

void BookCard::mousePressEvent(QMouseEvent*) {
    emit clicked(m_bookId);
}

void BookCard::enterEvent(QEnterEvent*) {
    m_hovered = true;
    update();
}

void BookCard::leaveEvent(QEvent*) {
    m_hovered = false;
    update();
}

void BookCard::paintEvent(QPaintEvent* e) {
    QWidget::paintEvent(e);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Hover glow around card
    if (m_hovered) {
        p.setPen(QPen(QColor("#c9a96e"), 2));
        QPainterPath path;
        path.addRoundedRect(QRectF(1, 1, 198, 198), 12, 12);
        p.drawPath(path);
    }

    // Progress bar at bottom of cover image
    if (m_progress > 0 && m_progress < 100) {
        int barY = 192;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 80));
        p.drawRoundedRect(0, barY, 200, 8, 4, 4);
        int w = m_progress * 2; // 200px wide = 100%
        QLinearGradient g(0, barY, w, barY);
        g.setColorAt(0, QColor("#c9a96e"));
        g.setColorAt(1, QColor("#f0c27f"));
        p.setBrush(g);
        p.drawRoundedRect(0, barY, w, 8, 4, 4);
    }
}
