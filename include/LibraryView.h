#pragma once
#include "Audiobook.h"
#include <QWidget>
#include <QMap>
#include <QScrollArea>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QProgressBar>
#include <QFlowLayout>

class BookCard;
class AudiobookScanner;

class LibraryView : public QWidget {
    Q_OBJECT
public:
    explicit LibraryView(QWidget* parent = nullptr);

    void openFolder(const QString& path);
    void updateBook(const Audiobook& book);   // refresh card after play
    const QMap<QString, Audiobook>& books() const { return m_books; }

signals:
    void bookSelected(const Audiobook& book);
    void scanStarted();
    void scanDone(int count);

private slots:
    void onBookFound(const Audiobook& book);
    void onScanFinished(int total);
    void onScanProgress(int current, int total);
    void applyFilter();

private:
    void setupUi();
    void rebuildGrid();

    QLineEdit*        m_searchBox;
    QComboBox*        m_sortBox;
    QLabel*           m_scanLabel;
    QProgressBar*     m_scanProgress;
    QWidget*          m_grid;
    QScrollArea*      m_scroll;
    AudiobookScanner* m_scanner;

    QMap<QString, Audiobook>  m_books;    // id → book
    QMap<QString, BookCard*>  m_cards;    // id → card widget
    QString                   m_filterText;
    int                       m_sortMode = 0; // 0=title 1=author 2=recent
};
