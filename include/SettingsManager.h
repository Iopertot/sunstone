#pragma once
#include <QObject>
#include <QSettings>
#include <QStringList>

class SettingsManager : public QObject {
    Q_OBJECT
public:
    explicit SettingsManager(QObject* parent = nullptr);

    QString     libraryPath() const;
    void        setLibraryPath(const QString& p);

    QStringList recentPaths() const;
    void        addRecentPath(const QString& p);

    float       defaultSpeed() const;
    void        setDefaultSpeed(float s);

    float       volume() const;
    void        setVolume(float v);

    int         skipSeconds() const;   // seconds for skip-back/forward
    void        setSkipSeconds(int s);

private:
    QSettings m_s;
};
