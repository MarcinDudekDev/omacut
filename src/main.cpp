// omacut — a dead-simple video length trimmer. Qt Quick (QML) UI, ffmpeg cuts.

#include <QtGlobal>

#ifdef Q_OS_LINUX
#include <QGuiApplication>
using OmacutApplication = QGuiApplication;
#else
// Everywhere but Linux the file dialogs are QtWidgets' native ones, and those
// need a QApplication rather than a bare QGuiApplication.
#include <QApplication>
using OmacutApplication = QApplication;
#endif

#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>

#include "backend.h"
#include "thumbprovider.h"

#ifdef Q_OS_MACOS
#include <QEvent>
#include <QFileOpenEvent>
#include <QObject>

namespace {
// Finder hands a double-clicked video, an "Open With", or a drop on the Dock
// icon to a Mac app as a QFileOpenEvent — never on the command line. Without
// this the app would open on an empty window and look broken.
class FileOpenFilter : public QObject {
public:
    FileOpenFilter(Backend *backend, QObject *parent)
        : QObject(parent), m_backend(backend) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        if (event->type() == QEvent::FileOpen) {
            const QUrl url = static_cast<QFileOpenEvent *>(event)->url();
            if (url.isLocalFile()) {
                m_backend->load(url);
                return true;
            }
        }
        return QObject::eventFilter(watched, event);
    }

private:
    Backend *m_backend;
};
}
#endif

int main(int argc, char *argv[]) {
    OmacutApplication app(argc, argv);
    app.setApplicationName("omacut");

    // Associates the window with omacut.desktop so the compositor (Wayland app_id
    // = this name) and taskbars pick up our installed icon.
    app.setDesktopFileName("omacut");
    app.setWindowIcon(QIcon::fromTheme("omacut"));

    // Modern, themeable controls (the same family Quickshell builds on).
    QQuickStyle::setStyle("Material");

    auto *provider = new ThumbProvider();
    Backend backend(provider, &app);

#ifdef Q_OS_MACOS
    app.installEventFilter(new FileOpenFilter(&backend, &app));
#endif

    QQmlApplicationEngine engine;

    // The engine takes ownership of the image provider.
    engine.addImageProvider("thumbs", provider);

    engine.rootContext()->setContextProperty("backend", &backend);

    engine.load(QUrl("qrc:/Main.qml"));
    if (engine.rootObjects().isEmpty())
        return -1;

    // Optionally open a file passed on the command line.
    const QStringList args = app.arguments();
    if (args.size() > 1)
        backend.load(QUrl::fromLocalFile(args.at(1)));

    return app.exec();
}
