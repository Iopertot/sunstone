#include "AudiobookScanner.h"
#include "MetadataReader.h"

#include <QDirIterator>
#include <QtConcurrent/QtConcurrentRun>
#include <QDebug>

AudiobookScanner::AudiobookScanner(QObject* parent) : QObject(parent) {}

void AudiobookScanner::scan(const QString& rootDir) {
    m_cancelled = false;
    QtConcurrent::run([this, rootDir]() { doScan(rootDir); });
}

void AudiobookScanner::cancel() { m_cancelled = true; }

void AudiobookScanner::doScan(const QString& root) {
    static const QStringList filters = {"*.m4b", "*.mp3", "*.m4a", "*.aax"};

    // First pass: collect paths
    QStringList paths;
    QDirIterator it(root, filters, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        if (m_cancelled) { emit scanFinished(0); return; }
        paths << it.next();
    }

    int total = paths.size();
    int current = 0;

    for (const QString& path : paths) {
        if (m_cancelled) break;
        Audiobook book;
        if (MetadataReader::read(path, book)) {
            emit bookFound(book);
        }
        ++current;
        emit scanProgress(current, total);
    }

    emit scanFinished(current);
}
