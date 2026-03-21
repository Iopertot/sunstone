#include "MainWindow.h"
#include <QApplication>
#include <QFont>
#include <QSurfaceFormat>

int main(int argc, char* argv[]) {
    // High-DPI support
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication app(argc, argv);
    app.setApplicationName("AudiobookPlayer");
    app.setOrganizationName("AudiobookPlayer");
    app.setApplicationVersion("1.0.0");

    // Default font
    QFont f = app.font();
    f.setFamily("system-ui");
    f.setPointSize(12);
    app.setFont(f);

    MainWindow w;
    w.show();
    return app.exec();
}
