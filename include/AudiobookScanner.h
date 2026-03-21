#pragma once
#include "Audiobook.h"
#include <QObject>
#include <QVector>
#include <QString>

// Scans a directory tree for .m4b / .mp3 / .m4a files,
// extracts metadata, and emits results progressively.
class AudiobookScanner : public QObject {
    Q_OBJECT
public:
    explicit AudiobookScanner(QObject* parent = nullptr);

    // Non-blocking: starts a thread that emits signals as books are found.
    void scan(const QString& rootDir);
    void cancel();

signals:
    void bookFound(Audiobook book);
    void scanFinished(int total);
    void scanProgress(int current, int total);

private:
    bool m_cancelled = false;
    void doScan(const QString& root);
};
