#include "PlayerView.h"
#include "AudioPlayer.h"
#include "BookmarkManager.h"
#include "ChapterList.h"
#include "WaveformWidget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QSlider>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>
#include <QInputDialog>
#include <QMessageBox>
#include <QFont>
#include <QTimerEvent>
#include <QListWidgetItem>

// ── helpers ──────────────────────────────────────────────────────────────────
static QString fmt(qint64 ms) {
    qint64 s = ms / 1000;
    int h = s/3600, m = (s%3600)/60, sec = s%60;
    if (h > 0)
        return QString("%1:%2:%3").arg(h).arg(m,2,10,QChar('0')).arg(sec,2,10,QChar('0'));
    return QString("%1:%2").arg(m).arg(sec,2,10,QChar('0'));
}

static QPushButton* iconBtn(const QString& text, const QString& tip, QWidget* parent) {
    auto* btn = new QPushButton(text, parent);
    btn->setToolTip(tip);
    btn->setFixedSize(48, 48);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(
        "QPushButton { background:transparent; color:#c8c0b0; font-size:20px; border:none; border-radius:24px; }"
        "QPushButton:hover { background:#2a2820; color:#f0c27f; }"
        "QPushButton:pressed { background:#3a3830; }");
    return btn;
}

static QPushButton* playBtn(QWidget* parent) {
    auto* btn = new QPushButton("▶", parent);
    btn->setFixedSize(64, 64);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(
        "QPushButton { background:#c9a96e; color:#0f0e0c; font-size:24px; "
        "  border:none; border-radius:32px; }"
        "QPushButton:hover { background:#f0c27f; }"
        "QPushButton:pressed { background:#a88040; }");
    return btn;
}

// ─────────────────────────────────────────────────────────────────────────────

PlayerView::PlayerView(AudioPlayer* player, BookmarkManager* bm, QWidget* parent)
    : QWidget(parent), m_player(player), m_bm(bm)
{
    setupUi();
    setupConnections();
}

void PlayerView::setupUi() {
    setStyleSheet("background:#100f0d;");
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0,0,0,0);
    root->setSpacing(0);

    // ─── Left panel: cover + controls ────────────────────────────────────────
    auto* left = new QWidget(this);
    left->setMinimumWidth(380);
    left->setMaximumWidth(460);
    left->setStyleSheet("background:#100f0d;");
    auto* lv = new QVBoxLayout(left);
    lv->setContentsMargins(40, 40, 40, 40);
    lv->setSpacing(16);

    // Back button
    m_btnBack = new QPushButton("← Library", left);
    m_btnBack->setStyleSheet(
        "QPushButton { background:transparent; color:#6b6557; font-size:12px; "
        "  border:none; text-align:left; }"
        "QPushButton:hover { color:#c9a96e; }");
    m_btnBack->setCursor(Qt::PointingHandCursor);
    lv->addWidget(m_btnBack, 0, Qt::AlignLeft);

    // Cover
    m_coverLabel = new QLabel(left);
    m_coverLabel->setFixedSize(300, 300);
    m_coverLabel->setAlignment(Qt::AlignCenter);
    m_coverLabel->setStyleSheet("border-radius:16px;");
    lv->addWidget(m_coverLabel, 0, Qt::AlignHCenter);

    // Title / author
    m_titleLabel = new QLabel(left);
    m_titleLabel->setStyleSheet(
        "color:#e8e4d9; font-family:'Georgia',serif; font-size:20px; font-weight:bold;");
    m_titleLabel->setWordWrap(true);
    m_titleLabel->setAlignment(Qt::AlignHCenter);

    m_authorLabel = new QLabel(left);
    m_authorLabel->setStyleSheet("color:#a09880; font-size:13px;");
    m_authorLabel->setAlignment(Qt::AlignHCenter);

    m_chapterLabel = new QLabel(left);
    m_chapterLabel->setStyleSheet("color:#6b6557; font-size:11px; letter-spacing:1px;");
    m_chapterLabel->setAlignment(Qt::AlignHCenter);

    lv->addWidget(m_titleLabel);
    lv->addWidget(m_authorLabel);
    lv->addWidget(m_chapterLabel);

    // Waveform / seek bar
    m_waveform = new WaveformWidget(left);
    lv->addWidget(m_waveform);

    // Position / duration labels
    auto* timeRow = new QHBoxLayout;
    m_posLabel = new QLabel("0:00", left);
    m_posLabel->setStyleSheet("color:#6b6557; font-size:11px;");
    m_durLabel = new QLabel("0:00", left);
    m_durLabel->setStyleSheet("color:#6b6557; font-size:11px;");
    timeRow->addWidget(m_posLabel);
    timeRow->addStretch();
    timeRow->addWidget(m_durLabel);
    lv->addLayout(timeRow);

    // Transport
    auto* transport = new QHBoxLayout;
    transport->setSpacing(8);
    m_btnSkipBack = iconBtn("⟨30", "Skip back 30s",  left);
    m_btnPlay     = playBtn(left);
    m_btnSkipFwd  = iconBtn("30⟩", "Skip forward 30s", left);
    m_btnBookmark = iconBtn("🔖", "Add bookmark", left);
    transport->addStretch();
    transport->addWidget(m_btnSkipBack);
    transport->addWidget(m_btnPlay);
    transport->addWidget(m_btnSkipFwd);
    transport->addWidget(m_btnBookmark);
    transport->addStretch();
    lv->addLayout(transport);

    // Speed row
    auto* speedRow = new QHBoxLayout;
    m_btnSpeedDn = new QPushButton("−", left);
    m_btnSpeedDn->setFixedSize(28, 28);
    m_btnSpeedDn->setStyleSheet(
        "QPushButton{background:#222018;color:#c8c0b0;border:1px solid #3a3830;"
        "border-radius:4px;} QPushButton:hover{background:#3a3830;}");

    m_speedLabel = new QLabel("1.0×", left);
    m_speedLabel->setStyleSheet("color:#c8c0b0; font-size:13px; min-width:40px;");
    m_speedLabel->setAlignment(Qt::AlignHCenter);

    m_btnSpeedUp = new QPushButton("+", left);
    m_btnSpeedUp->setFixedSize(28, 28);
    m_btnSpeedUp->setStyleSheet(m_btnSpeedDn->styleSheet());

    auto* volLabel = new QLabel("🔊", left);
    volLabel->setStyleSheet("color:#6b6557; font-size:14px;");
    m_volSlider = new QSlider(Qt::Horizontal, left);
    m_volSlider->setRange(0, 100);
    m_volSlider->setValue(80);
    m_volSlider->setFixedWidth(100);
    m_volSlider->setStyleSheet(
        "QSlider::groove:horizontal{background:#2a2820;height:4px;border-radius:2px;}"
        "QSlider::handle:horizontal{background:#c9a96e;width:14px;height:14px;"
        "  margin:-5px 0;border-radius:7px;}"
        "QSlider::sub-page:horizontal{background:#c9a96e;border-radius:2px;}");

    speedRow->addStretch();
    speedRow->addWidget(new QLabel("Speed:", left));
    speedRow->addWidget(m_btnSpeedDn);
    speedRow->addWidget(m_speedLabel);
    speedRow->addWidget(m_btnSpeedUp);
    speedRow->addSpacing(24);
    speedRow->addWidget(volLabel);
    speedRow->addWidget(m_volSlider);
    speedRow->addStretch();
    lv->addLayout(speedRow);

    lv->addStretch();

    // ─── Right panel: chapters + bookmarks ───────────────────────────────────
    auto* right = new QWidget(this);
    right->setStyleSheet("background:#0f0e0c; border-left:1px solid #1e1c18;");
    auto* rv = new QVBoxLayout(right);
    rv->setContentsMargins(0,0,0,0);
    rv->setSpacing(0);

    auto makePanelHeader = [](const QString& t, QWidget* p) {
        auto* w = new QWidget(p);
        w->setFixedHeight(40);
        w->setStyleSheet("background:#0f0e0c; border-bottom:1px solid #1e1c18;");
        auto* l = new QHBoxLayout(w);
        l->setContentsMargins(16,0,16,0);
        auto* lbl = new QLabel(t, w);
        lbl->setStyleSheet("color:#c9a96e; font-size:10px; letter-spacing:2px;");
        l->addWidget(lbl);
        return w;
    };

    // Chapters
    rv->addWidget(makePanelHeader("CHAPTERS", right));
    m_chapters = new ChapterList(right);
    rv->addWidget(m_chapters, 3);

    // Bookmarks
    rv->addWidget(makePanelHeader("BOOKMARKS", right));
    m_bmList = new QListWidget(right);
    m_bmList->setStyleSheet(
        "QListWidget{background:#0f0e0c;border:none;color:#c8c0b0;font-size:12px;}"
        "QListWidget::item{padding:8px 12px;border-bottom:1px solid #1a1814;}"
        "QListWidget::item:selected{background:#1e1c18;color:#c9a96e;}"
        "QListWidget::item:hover{background:#161410;}");
    rv->addWidget(m_bmList, 2);

    // Bookmark action bar
    auto* bmBar = new QWidget(right);
    bmBar->setFixedHeight(44);
    bmBar->setStyleSheet("background:#0a0908; border-top:1px solid #1e1c18;");
    auto* bmBarL = new QHBoxLayout(bmBar);
    bmBarL->setContentsMargins(12,4,12,4);
    m_btnDelBm = new QPushButton("Delete Selected", bmBar);
    m_btnDelBm->setStyleSheet(
        "QPushButton{background:#2a1818;color:#c05050;border:1px solid #3a2020;"
        "border-radius:4px;padding:4px 12px;font-size:11px;}"
        "QPushButton:hover{background:#3a2020;}");
    bmBarL->addStretch();
    bmBarL->addWidget(m_btnDelBm);
    rv->addWidget(bmBar);

    root->addWidget(left);
    root->addWidget(right, 1);
}

void PlayerView::setupConnections() {
    connect(m_btnBack, &QPushButton::clicked, this, &PlayerView::backToLibrary);

    connect(m_player, &AudioPlayer::positionChanged, this, &PlayerView::onPositionChanged);
    connect(m_player, &AudioPlayer::durationChanged, this, &PlayerView::onDurationChanged);
    connect(m_player, &AudioPlayer::playbackStateChanged, this, &PlayerView::onPlaybackStateChanged);

    connect(m_btnPlay, &QPushButton::clicked, this, [this](){
        if (m_player->isPlaying()) m_player->pause(); else m_player->play();
    });
    connect(m_btnSkipBack, &QPushButton::clicked, this, [this](){
        m_player->seekTo(qMax(0LL, m_player->positionMs() - 30000));
    });
    connect(m_btnSkipFwd, &QPushButton::clicked, this, [this](){
        m_player->seekTo(qMin(m_player->durationMs(), m_player->positionMs() + 30000));
    });
    connect(m_btnBookmark, &QPushButton::clicked, this, &PlayerView::onAddBookmark);
    connect(m_btnDelBm, &QPushButton::clicked, this, &PlayerView::onDeleteBookmark);

    connect(m_waveform, &WaveformWidget::seekRequested, m_player, &AudioPlayer::seekTo);

    connect(m_chapters, &ChapterList::chapterSelected, m_player, &AudioPlayer::seekTo);

    connect(m_bmList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item){
        qint64 ms = item->data(Qt::UserRole).toLongLong();
        m_player->seekTo(ms);
    });

    connect(m_volSlider, &QSlider::valueChanged, this, [this](int v){
        m_player->setVolume(v / 100.0f);
    });

    connect(m_btnSpeedDn, &QPushButton::clicked, this, [this](){
        m_speed = qMax(kSpeedMin, m_speed - kSpeedStep);
        m_player->setPlaybackRate(m_speed);
        updateSpeedLabel();
    });
    connect(m_btnSpeedUp, &QPushButton::clicked, this, [this](){
        m_speed = qMin(kSpeedMax, m_speed + kSpeedStep);
        m_player->setPlaybackRate(m_speed);
        updateSpeedLabel();
    });

    connect(m_bm, &BookmarkManager::bookmarksChanged, this, [this](const QString& id){
        if (id == m_book.id) refreshBookmarks();
    });

    // Auto-save every 15 seconds
    m_saveTimerId = startTimer(15000);
}

void PlayerView::loadBook(const Audiobook& book) {
    m_loading = true;
    m_book = book;

    // Cover
    QPixmap px = book.coverArt;
    if (px.isNull()) {
        px = QPixmap(300,300);
        px.fill(QColor("#1a1814"));
        QPainter p(&px);
        p.setPen(QColor(255,255,255,60));
        p.setFont(QFont("serif", 80));
        p.drawText(px.rect(), Qt::AlignCenter, "♫");
    } else {
        px = px.scaled(300, 300, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation)
               .copy(0, 0, 300, 300);
    }
    // Rounded
    QPixmap rounded(300,300);
    rounded.fill(Qt::transparent);
    QPainter pr(&rounded);
    pr.setRenderHint(QPainter::Antialiasing);
    QPainterPath path; path.addRoundedRect(0,0,300,300,16,16);
    pr.setClipPath(path);
    pr.drawPixmap(0,0,px);
    m_coverLabel->setPixmap(rounded);

    m_titleLabel->setText(book.title);
    m_authorLabel->setText(book.author);
    m_chapters->setChapters(book.chapters);

    m_player->setSource(book.filePath);

    // Restore position
    qint64 savedPos = m_bm->loadProgress(book.id);
    if (savedPos > 0) {
        QTimer::singleShot(500, this, [this, savedPos](){
            m_player->seekTo(savedPos);
        });
    }

    m_speed = book.playbackSpeed;
    m_player->setPlaybackRate(m_speed);
    updateSpeedLabel();

    refreshBookmarks();
    m_loading = false;
    m_player->play();
}

void PlayerView::onPositionChanged(qint64 ms) {
    m_posLabel->setText(fmt(ms));
    m_waveform->setPosition(ms);
    m_chapters->highlightCurrent(ms);
    if (!m_book.chapters.isEmpty()) {
        for (const auto& ch : m_book.chapters) {
            if (ms >= ch.startMs && (ch.endMs == 0 || ms < ch.endMs)) {
                m_chapterLabel->setText(ch.title.toUpper());
                break;
            }
        }
    }
}

void PlayerView::onDurationChanged(qint64 ms) {
    m_durLabel->setText(fmt(ms));
    m_waveform->setDuration(ms);
}

void PlayerView::onPlaybackStateChanged(bool playing) {
    m_btnPlay->setText(playing ? "⏸" : "▶");
}

void PlayerView::onAddBookmark() {
    bool ok;
    QString note = QInputDialog::getText(this, "Add Bookmark",
        "Note (optional):", QLineEdit::Normal, "", &ok);
    if (!ok) return;
    Bookmark bm;
    bm.audiobookId = m_book.id;
    bm.positionMs  = m_player->positionMs();
    bm.note        = note;
    m_bm->addBookmark(bm);
}

void PlayerView::onDeleteBookmark() {
    auto* item = m_bmList->currentItem();
    if (!item) return;
    int id = item->data(Qt::UserRole + 1).toInt();
    if (QMessageBox::question(this, "Delete Bookmark",
            "Delete this bookmark?") == QMessageBox::Yes)
        m_bm->removeBookmark(id);
}

void PlayerView::refreshBookmarks() {
    m_bmList->clear();
    QList<qint64> positions;
    for (const auto& bm : m_bm->bookmarksFor(m_book.id)) {
        QString label = bm.positionString();
        if (!bm.note.isEmpty()) label += "  —  " + bm.note;
        auto* item = new QListWidgetItem(label, m_bmList);
        item->setData(Qt::UserRole, bm.positionMs);
        item->setData(Qt::UserRole + 1, bm.id);
        positions << bm.positionMs;
    }
    m_waveform->setBookmarks(positions);
}

void PlayerView::updateSpeedLabel() {
    m_speedLabel->setText(QString("%1×").arg(m_speed, 0, 'f', 2));
}

void PlayerView::timerEvent(QTimerEvent* e) {
    if (e->timerId() == m_saveTimerId && m_player->isPlaying()) {
        m_bm->saveProgress(m_book.id, m_player->positionMs());
        m_book.lastPositionMs = m_player->positionMs();
        emit progressSaved(m_book);
    }
}

QString PlayerView::formatMs(qint64 ms) const { return fmt(ms); }
