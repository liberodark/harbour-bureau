#include <sailfishapp.h>

#include <QDir>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickView>
#include <QStandardPaths>

#include "appmodel.h"
#include "homemodel.h"
#include "systembridge.h"

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> application(SailfishApp::application(argc, argv));

    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dataDir.isEmpty())
        dataDir = QDir::homePath() + QStringLiteral("/.local/share/harbour-bureau");
    QDir().mkpath(dataDir);

    AppModel appModel;
    HomeModel homeModel(&appModel, dataDir + QStringLiteral("/layout.json"));
    homeModel.load();
    SystemBridge system;

    QScopedPointer<QQuickView> view(SailfishApp::createView());
    view->rootContext()->setContextProperty(QStringLiteral("appModel"), &appModel);
    view->rootContext()->setContextProperty(QStringLiteral("homeModel"), &homeModel);
    view->rootContext()->setContextProperty(QStringLiteral("bridge"), &system);
    view->setSource(SailfishApp::pathToMainQml());
    view->show();

    return application->exec();
}
