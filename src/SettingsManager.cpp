#include "SettingsManager.h"

SettingsManager::SettingsManager(QObject* parent)
    : QObject(parent)
    , m_s("AudiobookPlayer", "AudiobookPlayer")
{}

QString SettingsManager::libraryPath() const {
    return m_s.value("library/path", "").toString();
}
void SettingsManager::setLibraryPath(const QString& p) {
    m_s.setValue("library/path", p);
    addRecentPath(p);
}

QStringList SettingsManager::recentPaths() const {
    return m_s.value("library/recent").toStringList();
}
void SettingsManager::addRecentPath(const QString& p) {
    QStringList list = recentPaths();
    list.removeAll(p);
    list.prepend(p);
    while (list.size() > 8) list.removeLast();
    m_s.setValue("library/recent", list);
}

float SettingsManager::defaultSpeed() const {
    return m_s.value("playback/speed", 1.0f).toFloat();
}
void SettingsManager::setDefaultSpeed(float s) {
    m_s.setValue("playback/speed", s);
}

float SettingsManager::volume() const {
    return m_s.value("playback/volume", 0.8f).toFloat();
}
void SettingsManager::setVolume(float v) {
    m_s.setValue("playback/volume", v);
}

int SettingsManager::skipSeconds() const {
    return m_s.value("playback/skipSeconds", 30).toInt();
}
void SettingsManager::setSkipSeconds(int s) {
    m_s.setValue("playback/skipSeconds", s);
}
