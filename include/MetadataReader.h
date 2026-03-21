#pragma once
#include "Audiobook.h"
#include <QString>

class MetadataReader {
public:
    // Reads all metadata (title, author, chapters, cover art, duration)
    // from an m4b or mp3 file using FFmpeg. Returns false on failure.
    static bool read(const QString& filePath, Audiobook& out);

private:
    static QString sanitize(const char* raw);
};
