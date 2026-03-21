#include "MetadataReader.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
}

#include <QCryptographicHash>
#include <QPixmap>
#include <QFileInfo>
#include <QDebug>

static QString safeTag(AVDictionary* meta, const char* key) {
    if (!meta) return {};
    AVDictionaryEntry* e = av_dict_get(meta, key, nullptr, AV_DICT_IGNORE_SUFFIX);
    if (e && e->value) return QString::fromUtf8(e->value).trimmed();
    return {};
}

bool MetadataReader::read(const QString& filePath, Audiobook& out) {
    AVFormatContext* fmt = nullptr;
    QByteArray pathBytes = filePath.toUtf8();

    if (avformat_open_input(&fmt, pathBytes.constData(), nullptr, nullptr) < 0) {
        qWarning() << "Cannot open:" << filePath;
        return false;
    }

    if (avformat_find_stream_info(fmt, nullptr) < 0) {
        avformat_close_input(&fmt);
        return false;
    }

    // Duration
    if (fmt->duration != AV_NOPTS_VALUE)
        out.durationMs = fmt->duration / 1000;

    // File ID
    out.id = QCryptographicHash::hash(filePath.toUtf8(),
                                       QCryptographicHash::Sha1).toHex();
    out.filePath = filePath;

    // Global tags
    AVDictionary* meta = fmt->metadata;
    out.title    = safeTag(meta, "title");
    out.author   = safeTag(meta, "artist");
    if (out.author.isEmpty()) out.author = safeTag(meta, "album_artist");
    out.narrator = safeTag(meta, "composer");
    out.series   = safeTag(meta, "album");

    QString yearStr = safeTag(meta, "date");
    if (!yearStr.isEmpty()) out.year = yearStr.left(4).toInt();

    out.description = safeTag(meta, "comment");
    if (out.description.isEmpty())
        out.description = safeTag(meta, "description");

    // Fallback title from filename
    if (out.title.isEmpty())
        out.title = QFileInfo(filePath).completeBaseName();

    // Chapters
    for (unsigned i = 0; i < fmt->nb_chapters; ++i) {
        AVChapter* ch = fmt->chapters[i];
        Chapter chapter;
        AVDictionaryEntry* titleEntry = av_dict_get(ch->metadata, "title", nullptr, 0);
        chapter.title   = titleEntry ? QString::fromUtf8(titleEntry->value) : QString("Chapter %1").arg(i+1);
        double tbq      = av_q2d(ch->time_base);
        chapter.startMs = static_cast<qint64>(ch->start * tbq * 1000.0);
        chapter.endMs   = static_cast<qint64>(ch->end   * tbq * 1000.0);
        out.chapters.append(chapter);
    }

    // Cover art - look for attached picture stream
    for (unsigned i = 0; i < fmt->nb_streams; ++i) {
        AVStream* st = fmt->streams[i];
        if (st->disposition & AV_DISPOSITION_ATTACHED_PIC) {
            AVPacket& pkt = st->attached_pic;
            if (pkt.size > 0) {
                out.coverArtData = QByteArray(reinterpret_cast<char*>(pkt.data), pkt.size);
                out.coverArt.loadFromData(out.coverArtData);
            }
            break;
        }
    }

    avformat_close_input(&fmt);
    return true;
}
