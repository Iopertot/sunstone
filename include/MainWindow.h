#pragma once
#include <QMainWindow>
#include <QStackedWidget>

class LibraryView;
class PlayerView;
class AudioPlayer;
class BookmarkManager;
class SettingsManager;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private slots:
    void openLibraryFolder();
    void showLibrary();
    void showPlayer();

private:
    void setupUi();
    void setupMenuBar();
    void restoreState();

    QStackedWidget*  m_stack;
    LibraryView*     m_library;
    PlayerView*      m_player;
    AudioPlayer*     m_audio;
    BookmarkManager* m_bm;
    SettingsManager* m_settings;
};
