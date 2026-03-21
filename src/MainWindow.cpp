#include "MainWindow.h"
#include "LibraryView.h"
#include "PlayerView.h"
#include "AudioPlayer.h"
#include "BookmarkManager.h"
#include "SettingsManager.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QStandardPaths>
#include <QStackedWidget>
#include <QStatusBar>
#include <QLabel>
#include <QCloseEvent>
#include <QDir>
#include <QApplication>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_stack(new QStackedWidget(this))
    , m_audio(new AudioPlayer(this))
    , m_bm(new BookmarkManager(this))
    , m_settings(new SettingsManager(this))
{
    // Init DB
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    m_bm->init(dataDir + "/library.db");

    m_library = new LibraryView(this);
    m_player  = new PlayerView(m_audio, m_bm, this);

    setupUi();
    setupMenuBar();
    restoreState();

    // Signals
    connect(m_library, &LibraryView::bookSelected, this, [this](const Audiobook& book){
        m_player->loadBook(book);
        showPlayer();
    });
    connect(m_player, &PlayerView::backToLibrary, this, &MainWindow::showLibrary);
    connect(m_player, &PlayerView::progressSaved, this, [this](const Audiobook& book){
        m_library->updateBook(book);
    });
    connect(m_audio, &AudioPlayer::mediaError, this, [this](const QString& msg){
        statusBar()->showMessage("Playback error: " + msg, 5000);
    });
}

MainWindow::~MainWindow() {}

void MainWindow::setupUi() {
    setWindowTitle("Audiobook Player");
    resize(1200, 800);
    setMinimumSize(900, 600);

    // Dark title bar feel
    setStyleSheet("QMainWindow { background:#100f0d; }"
                  "QMenuBar { background:#0a0908; color:#a09880; font-size:12px; }"
                  "QMenuBar::item:selected { background:#1e1c18; color:#c9a96e; }"
                  "QMenu { background:#1a1814; color:#c8c0b0; border:1px solid #2a2820; }"
                  "QMenu::item:selected { background:#2d2820; color:#c9a96e; }"
                  "QStatusBar { background:#0a0908; color:#6b6557; font-size:11px; }");

    m_stack->addWidget(m_library);
    m_stack->addWidget(m_player);
    setCentralWidget(m_stack);
    showLibrary();
}

void MainWindow::setupMenuBar() {
    auto* fileMenu = menuBar()->addMenu("File");

    auto* openAct = fileMenu->addAction("Open Library Folder…");
    openAct->setShortcut(QKeySequence::Open);
    connect(openAct, &QAction::triggered, this, &MainWindow::openLibraryFolder);

    // Recent folders submenu
    auto* recentMenu = fileMenu->addMenu("Recent Folders");
    connect(recentMenu, &QMenu::aboutToShow, this, [this, recentMenu](){
        recentMenu->clear();
        for (const QString& path : m_settings->recentPaths()) {
            auto* act = recentMenu->addAction(path);
            connect(act, &QAction::triggered, this, [this, path](){
                m_settings->setLibraryPath(path);
                m_library->openFolder(path);
                showLibrary();
            });
        }
    });

    fileMenu->addSeparator();
    auto* quitAct = fileMenu->addAction("Quit");
    quitAct->setShortcut(QKeySequence::Quit);
    connect(quitAct, &QAction::triggered, qApp, &QApplication::quit);

    auto* viewMenu = menuBar()->addMenu("View");
    auto* libAct = viewMenu->addAction("Library");
    libAct->setShortcut(Qt::Key_Escape);
    connect(libAct, &QAction::triggered, this, &MainWindow::showLibrary);

    auto* helpMenu = menuBar()->addMenu("Help");
    auto* aboutAct = helpMenu->addAction("About Audiobook Player");
    connect(aboutAct, &QAction::triggered, this, [this](){
        QMessageBox::about(this, "Audiobook Player",
            "<h3>Audiobook Player</h3>"
            "<p>A beautiful audiobook player supporting M4B, MP3, and M4A files.</p>"
            "<p>Features: library management, bookmarks, chapter navigation, "
            "variable speed playback.</p>");
    });
}

void MainWindow::restoreState() {
    QString libPath = m_settings->libraryPath();
    if (!libPath.isEmpty() && QDir(libPath).exists())
        m_library->openFolder(libPath);
}

void MainWindow::openLibraryFolder() {
    QString dir = QFileDialog::getExistingDirectory(
        this, "Open Audiobook Library",
        m_settings->libraryPath().isEmpty()
            ? QStandardPaths::writableLocation(QStandardPaths::MusicLocation)
            : m_settings->libraryPath());
    if (dir.isEmpty()) return;
    m_settings->setLibraryPath(dir);
    m_library->openFolder(dir);
    showLibrary();
}

void MainWindow::showLibrary() { m_stack->setCurrentWidget(m_library); }
void MainWindow::showPlayer()  { m_stack->setCurrentWidget(m_player); }

void MainWindow::closeEvent(QCloseEvent* e) {
    // Save current playback position before quitting
    if (m_audio->isPlaying()) {
        m_bm->saveProgress(m_player->currentBook().id, m_audio->positionMs());
    }
    e->accept();
}

void MainWindow::resizeEvent(QResizeEvent* e) {
    QMainWindow::resizeEvent(e);
}
