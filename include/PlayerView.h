#pragma once
#include "Audiobook.h"
#include "Bookmark.h"
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QListWidget>

class AudioPlayer;
class BookmarkManager;
class ChapterList;
class WaveformWidget;

class PlayerView : public QWidget {
    Q_OBJECT
public:
    explicit PlayerView(AudioPlayer* player, BookmarkManager* bm, QWidget* parent = nullptr);

    void loadBook(const Audiobook& book);
    const Audiobook& currentBook() const { return m_book; }

signals:
    void backToLibrary();
    void progressSaved(const Audiobook& book); // emitted periodically

private slots:
    void onPositionChanged(qint64 ms);
    void onDurationChanged(qint64 ms);
    void onPlaybackStateChanged(bool playing);
    void onAddBookmark();
    void onDeleteBookmark();
    void refreshBookmarks();
    void updateSpeedLabel();

private:
    void setupUi();
    void setupConnections();
    QString formatMs(qint64 ms) const;

    AudioPlayer*     m_player;
    BookmarkManager* m_bm;
    Audiobook        m_book;
    bool             m_loading = false;

    // Cover
    QLabel*        m_coverLabel;

    // Text
    QLabel*        m_titleLabel;
    QLabel*        m_authorLabel;
    QLabel*        m_chapterLabel;

    // Seek / waveform
    WaveformWidget* m_waveform;
    QLabel*         m_posLabel;
    QLabel*         m_durLabel;

    // Transport controls
    QPushButton*   m_btnBack;      // back to library
    QPushButton*   m_btnSkipBack;  // -30s
    QPushButton*   m_btnPlay;
    QPushButton*   m_btnSkipFwd;   // +30s
    QPushButton*   m_btnBookmark;

    // Speed
    QPushButton*   m_btnSpeedDn;
    QLabel*        m_speedLabel;
    QPushButton*   m_btnSpeedUp;

    // Volume
    QSlider*       m_volSlider;

    // Chapters
    ChapterList*   m_chapters;

    // Bookmarks
    QListWidget*   m_bmList;
    QPushButton*   m_btnDelBm;

    // Timer for auto-save
    int            m_saveTimerId = -1;
    float          m_speed = 1.0f;
    static constexpr float kSpeedStep = 0.25f;
    static constexpr float kSpeedMin  = 0.5f;
    static constexpr float kSpeedMax  = 3.0f;
};
