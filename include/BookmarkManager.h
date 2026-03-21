#pragma once
#include "Bookmark.h"
#include <QVector>
#include <QObject>

class QSqlDatabase;

class BookmarkManager : public QObject {
    Q_OBJECT
public:
    explicit BookmarkManager(QObject* parent = nullptr);

    bool        init(const QString& dbPath);
    QVector<Bookmark> bookmarksFor(const QString& audiobookId);
    bool        addBookmark(Bookmark& bm);   // sets bm.id on success
    bool        updateBookmark(const Bookmark& bm);
    bool        removeBookmark(int id);

    // Persist last playback position
    bool        saveProgress(const QString& audiobookId, qint64 posMs);
    qint64      loadProgress(const QString& audiobookId);

signals:
    void        bookmarksChanged(const QString& audiobookId);

private:
    void        createTables();
};
