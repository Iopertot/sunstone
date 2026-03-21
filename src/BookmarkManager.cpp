#include "BookmarkManager.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QVariant>

static const char* DB_CONN = "audiobook_db";

BookmarkManager::BookmarkManager(QObject* parent) : QObject(parent) {}

bool BookmarkManager::init(const QString& dbPath) {
    auto db = QSqlDatabase::addDatabase("QSQLITE", DB_CONN);
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        qCritical() << "Cannot open DB:" << db.lastError().text();
        return false;
    }
    createTables();
    return true;
}

void BookmarkManager::createTables() {
    QSqlDatabase db = QSqlDatabase::database(DB_CONN);
    QSqlQuery q(db);
    q.exec(R"(CREATE TABLE IF NOT EXISTS bookmarks (
        id          INTEGER PRIMARY KEY AUTOINCREMENT,
        book_id     TEXT NOT NULL,
        position_ms INTEGER NOT NULL,
        note        TEXT,
        created_at  TEXT NOT NULL
    ))");
    q.exec(R"(CREATE TABLE IF NOT EXISTS progress (
        book_id     TEXT PRIMARY KEY,
        position_ms INTEGER NOT NULL
    ))");
}

QVector<Bookmark> BookmarkManager::bookmarksFor(const QString& audiobookId) {
    QVector<Bookmark> result;
    QSqlDatabase db = QSqlDatabase::database(DB_CONN);
    QSqlQuery q(db);
    q.prepare("SELECT id, book_id, position_ms, note, created_at "
              "FROM bookmarks WHERE book_id=? ORDER BY position_ms");
    q.addBindValue(audiobookId);
    if (!q.exec()) return result;
    while (q.next()) {
        Bookmark bm;
        bm.id          = q.value(0).toInt();
        bm.audiobookId = q.value(1).toString();
        bm.positionMs  = q.value(2).toLongLong();
        bm.note        = q.value(3).toString();
        bm.createdAt   = QDateTime::fromString(q.value(4).toString(), Qt::ISODate);
        result.append(bm);
    }
    return result;
}

bool BookmarkManager::addBookmark(Bookmark& bm) {
    QSqlDatabase db = QSqlDatabase::database(DB_CONN);
    QSqlQuery q(db);
    bm.createdAt = QDateTime::currentDateTime();
    q.prepare("INSERT INTO bookmarks (book_id, position_ms, note, created_at) "
              "VALUES (?,?,?,?)");
    q.addBindValue(bm.audiobookId);
    q.addBindValue(bm.positionMs);
    q.addBindValue(bm.note);
    q.addBindValue(bm.createdAt.toString(Qt::ISODate));
    if (!q.exec()) { qWarning() << q.lastError(); return false; }
    bm.id = q.lastInsertId().toInt();
    emit bookmarksChanged(bm.audiobookId);
    return true;
}

bool BookmarkManager::updateBookmark(const Bookmark& bm) {
    QSqlDatabase db = QSqlDatabase::database(DB_CONN);
    QSqlQuery q(db);
    q.prepare("UPDATE bookmarks SET note=? WHERE id=?");
    q.addBindValue(bm.note);
    q.addBindValue(bm.id);
    if (!q.exec()) return false;
    emit bookmarksChanged(bm.audiobookId);
    return true;
}

bool BookmarkManager::removeBookmark(int id) {
    QSqlDatabase db = QSqlDatabase::database(DB_CONN);
    QSqlQuery q(db);
    // fetch book_id first for signal
    q.prepare("SELECT book_id FROM bookmarks WHERE id=?");
    q.addBindValue(id);
    q.exec(); q.next();
    QString bookId = q.value(0).toString();
    q.prepare("DELETE FROM bookmarks WHERE id=?");
    q.addBindValue(id);
    if (!q.exec()) return false;
    emit bookmarksChanged(bookId);
    return true;
}

bool BookmarkManager::saveProgress(const QString& audiobookId, qint64 posMs) {
    QSqlDatabase db = QSqlDatabase::database(DB_CONN);
    QSqlQuery q(db);
    q.prepare("INSERT OR REPLACE INTO progress (book_id, position_ms) VALUES (?,?)");
    q.addBindValue(audiobookId);
    q.addBindValue(posMs);
    return q.exec();
}

qint64 BookmarkManager::loadProgress(const QString& audiobookId) {
    QSqlDatabase db = QSqlDatabase::database(DB_CONN);
    QSqlQuery q(db);
    q.prepare("SELECT position_ms FROM progress WHERE book_id=?");
    q.addBindValue(audiobookId);
    if (q.exec() && q.next()) return q.value(0).toLongLong();
    return 0;
}
