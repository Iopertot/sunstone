#include "LibraryView.h"
#include "BookCard.h"
#include "AudiobookScanner.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QProgressBar>
#include <QScrollBar>
#include <QResizeEvent>
#include <QWrapLayout>   // see note – we implement a simple flow below

// ── Simple flow layout (avoids external dependency) ─────────────────────────
#include <QLayout>
#include <QStyle>

class FlowLayout : public QLayout {
public:
    explicit FlowLayout(QWidget* parent, int hSpacing = 24, int vSpacing = 24)
        : QLayout(parent), m_hSpace(hSpacing), m_vSpace(vSpacing) {}
    ~FlowLayout() { QLayoutItem* item; while ((item = takeAt(0))) delete item; }
    void addItem(QLayoutItem* item) override { m_items.append(item); }
    int  count() const override { return m_items.size(); }
    QLayoutItem* itemAt(int i) const override { return m_items.value(i); }
    QLayoutItem* takeAt(int i) override {
        return i >= 0 && i < m_items.size() ? m_items.takeAt(i) : nullptr;
    }
    Qt::Orientations expandingDirections() const override { return {}; }
    bool hasHeightForWidth() const override { return true; }
    int  heightForWidth(int w) const override { return doLayout(QRect(0,0,w,0), true); }
    QSize sizeHint() const override { return minimumSize(); }
    QSize minimumSize() const override {
        QSize s;
        for (auto* i : m_items) s = s.expandedTo(i->minimumSize());
        const QMargins mg = contentsMargins();
        return s + QSize(mg.left()+mg.right(), mg.top()+mg.bottom());
    }
    void setGeometry(const QRect& r) override {
        QLayout::setGeometry(r);
        doLayout(r, false);
    }
private:
    int doLayout(const QRect& rect, bool testOnly) const {
        const QMargins mg = contentsMargins();
        QRect effective = rect.adjusted(mg.left(), mg.top(), -mg.right(), -mg.bottom());
        int x = effective.x(), y = effective.y(), lineH = 0;
        for (auto* item : m_items) {
            QSize s = item->sizeHint();
            int nextX = x + s.width() + m_hSpace;
            if (nextX - m_hSpace > effective.right() && lineH > 0) {
                x = effective.x(); y += lineH + m_vSpace; nextX = x + s.width() + m_hSpace; lineH = 0;
            }
            if (!testOnly) item->setGeometry(QRect(QPoint(x, y), s));
            x = nextX;
            lineH = qMax(lineH, s.height());
        }
        return y + lineH - rect.y() + mg.bottom();
    }
    QList<QLayoutItem*> m_items;
    int m_hSpace, m_vSpace;
};
// ─────────────────────────────────────────────────────────────────────────────

LibraryView::LibraryView(QWidget* parent)
    : QWidget(parent)
    , m_scanner(new AudiobookScanner(this))
{
    setupUi();

    connect(m_scanner, &AudiobookScanner::bookFound,
            this, &LibraryView::onBookFound);
    connect(m_scanner, &AudiobookScanner::scanFinished,
            this, &LibraryView::onScanFinished);
    connect(m_scanner, &AudiobookScanner::scanProgress,
            this, &LibraryView::onScanProgress);
    connect(m_searchBox, &QLineEdit::textChanged,
            this, [this](const QString& t){ m_filterText = t; applyFilter(); });
    connect(m_sortBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int i){ m_sortMode = i; rebuildGrid(); });
}

void LibraryView::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Toolbar ──────────────────────────────────────────────────────────────
    auto* bar = new QWidget(this);
    bar->setFixedHeight(56);
    bar->setStyleSheet("background:#161410; border-bottom:1px solid #2a2820;");
    auto* barL = new QHBoxLayout(bar);
    barL->setContentsMargins(20, 8, 20, 8);

    auto* libLabel = new QLabel("MY LIBRARY", bar);
    libLabel->setStyleSheet("color:#c9a96e; font-family:'Georgia',serif; "
                             "font-size:11px; letter-spacing:3px;");

    m_searchBox = new QLineEdit(bar);
    m_searchBox->setPlaceholderText("Search titles, authors…");
    m_searchBox->setFixedWidth(240);
    m_searchBox->setStyleSheet(
        "QLineEdit { background:#222018; border:1px solid #3a3830; border-radius:4px;"
        "  color:#c8c0b0; padding:4px 10px; font-size:12px; }"
        "QLineEdit:focus { border-color:#c9a96e; }");

    m_sortBox = new QComboBox(bar);
    m_sortBox->addItems({"Sort: Title", "Sort: Author", "Sort: Recently Played"});
    m_sortBox->setStyleSheet(
        "QComboBox { background:#222018; border:1px solid #3a3830; border-radius:4px;"
        "  color:#c8c0b0; padding:4px 10px; font-size:12px; }"
        "QComboBox::drop-down { border:none; }"
        "QComboBox QAbstractItemView { background:#222018; color:#c8c0b0; "
        "  selection-background-color:#2d2820; }");

    barL->addWidget(libLabel);
    barL->addStretch();
    barL->addWidget(m_searchBox);
    barL->addSpacing(12);
    barL->addWidget(m_sortBox);

    // ── Scan progress ─────────────────────────────────────────────────────────
    auto* scanBar = new QWidget(this);
    scanBar->setFixedHeight(32);
    scanBar->setStyleSheet("background:#0f0e0c;");
    auto* scanL = new QHBoxLayout(scanBar);
    scanL->setContentsMargins(20, 4, 20, 4);

    m_scanLabel = new QLabel("", scanBar);
    m_scanLabel->setStyleSheet("color:#6b6557; font-size:11px;");
    m_scanProgress = new QProgressBar(scanBar);
    m_scanProgress->setFixedWidth(200);
    m_scanProgress->setFixedHeight(4);
    m_scanProgress->setTextVisible(false);
    m_scanProgress->setStyleSheet(
        "QProgressBar { background:#2a2820; border:none; border-radius:2px; }"
        "QProgressBar::chunk { background:#c9a96e; border-radius:2px; }");
    m_scanProgress->hide();

    scanL->addWidget(m_scanLabel);
    scanL->addStretch();
    scanL->addWidget(m_scanProgress);

    // ── Grid scroll area ──────────────────────────────────────────────────────
    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setStyleSheet(
        "QScrollArea { background:#100f0d; border:none; }"
        "QScrollBar:vertical { background:#1a1814; width:8px; }"
        "QScrollBar::handle:vertical { background:#3a3830; border-radius:4px; min-height:30px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0; }");

    m_grid = new QWidget;
    m_grid->setStyleSheet("background:#100f0d;");
    auto* flow = new FlowLayout(m_grid, 24, 32);
    flow->setContentsMargins(32, 32, 32, 32);
    m_scroll->setWidget(m_grid);

    root->addWidget(bar);
    root->addWidget(scanBar);
    root->addWidget(m_scroll, 1);
}

void LibraryView::openFolder(const QString& path) {
    m_books.clear();
    for (auto* c : m_cards) delete c;
    m_cards.clear();
    m_scanLabel->setText("Scanning…");
    m_scanProgress->show();
    m_scanProgress->setRange(0, 0);
    emit scanStarted();
    m_scanner->scan(path);
}

void LibraryView::onBookFound(const Audiobook& book) {
    m_books[book.id] = book;
    auto* card = new BookCard(book, m_grid);
    m_cards[book.id] = card;
    m_grid->layout()->addWidget(card);
    connect(card, &BookCard::clicked, this, [this](const QString& id){
        emit bookSelected(m_books[id]);
    });
    applyFilter();
}

void LibraryView::onScanFinished(int total) {
    m_scanLabel->setText(QString("%1 audiobooks").arg(total));
    m_scanProgress->hide();
    emit scanDone(total);
    rebuildGrid();
}

void LibraryView::onScanProgress(int current, int total) {
    if (m_scanProgress->maximum() == 0) {
        m_scanProgress->setRange(0, total);
    }
    m_scanProgress->setValue(current);
    m_scanLabel->setText(QString("Scanning… %1 / %2").arg(current).arg(total));
}

void LibraryView::updateBook(const Audiobook& book) {
    m_books[book.id] = book;
    if (m_cards.contains(book.id))
        m_cards[book.id]->updateBook(book);
}

void LibraryView::applyFilter() {
    QString f = m_filterText.toLower();
    for (auto it = m_cards.begin(); it != m_cards.end(); ++it) {
        const Audiobook& b = m_books[it.key()];
        bool match = f.isEmpty()
            || b.title.toLower().contains(f)
            || b.author.toLower().contains(f);
        it.value()->setVisible(match);
    }
}

void LibraryView::rebuildGrid() {
    // Sort cards by chosen mode
    QList<Audiobook> sorted = m_books.values();
    if (m_sortMode == 0)
        std::sort(sorted.begin(), sorted.end(), [](const Audiobook& a, const Audiobook& b){
            return a.title.toLower() < b.title.toLower();
        });
    else if (m_sortMode == 1)
        std::sort(sorted.begin(), sorted.end(), [](const Audiobook& a, const Audiobook& b){
            return a.author.toLower() < b.author.toLower();
        });
    else
        std::sort(sorted.begin(), sorted.end(), [](const Audiobook& a, const Audiobook& b){
            return a.lastPlayed > b.lastPlayed;
        });

    // Re-add in order
    QLayout* flow = m_grid->layout();
    for (const auto& book : sorted) {
        if (m_cards.contains(book.id))
            flow->addWidget(m_cards[book.id]);
    }
    applyFilter();
}
