#include "../src/appmodel.h"
#include "../src/homemodel.h"
#include "../src/systembridge.h"

#include <QDirIterator>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlExpression>
#include <QQuickItem>
#include <QQuickView>
#include <QTemporaryDir>
#include <QTimer>

#include <cstdio>

static QStringList messages;

static void handler(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    const char *level = type == QtWarningMsg ? "WARN" : type == QtCriticalMsg ? "CRIT"
                      : type == QtFatalMsg ? "FATAL" : "info";
    if (type != QtDebugMsg && type != QtInfoMsg)
        messages.append(msg);
    std::fprintf(stderr, "[%s] %s\n", level, qPrintable(msg));
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(handler);
    QGuiApplication app(argc, argv);
    const QString qmlDir = QString::fromLocal8Bit(argv[1]);

    QTemporaryDir tmp;
    AppModel appModel;
    HomeModel homeModel(&appModel, tmp.path() + QStringLiteral("/layout.json"));
    SystemBridge bridge;

    QQuickView view;
    QQmlContext *ctx = view.rootContext();
    ctx->setContextProperty(QStringLiteral("appModel"), &appModel);
    ctx->setContextProperty(QStringLiteral("homeModel"), &homeModel);
    ctx->setContextProperty(QStringLiteral("bridge"), &bridge);

    int errors = 0;

    QDirIterator it(qmlDir, QStringList() << QStringLiteral("*.qml"), QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString file = it.next();
        QQmlComponent component(view.engine(), QUrl::fromLocalFile(file));
        if (component.isError()) {
            ++errors;
            for (const QQmlError &e : component.errors())
                std::fprintf(stderr, "ERROR %s\n", qPrintable(e.toString()));
        } else {
            std::fprintf(stderr, "compiled %s\n", qPrintable(file));
        }
    }

    homeModel.addApps(appModel.rowCount() > 0
                      ? QStringList() << appModel.data(appModel.index(0), AppModel::DesktopPathRole).toString()
                      : QStringList());
    int folderId = homeModel.createFolder(QStringLiteral("Test"), QStringList());
    if (appModel.rowCount() > 2) {
        homeModel.addToFolder(folderId, QStringList()
                              << appModel.data(appModel.index(1), AppModel::DesktopPathRole).toString()
                              << appModel.data(appModel.index(2), AppModel::DesktopPathRole).toString());
    }

    view.setSource(QUrl::fromLocalFile(qmlDir + QStringLiteral("/harbour-bureau.qml")));
    if (view.status() == QQuickView::Error) {
        ++errors;
        for (const QQmlError &e : view.errors())
            std::fprintf(stderr, "ERROR %s\n", qPrintable(e.toString()));
    }
    view.resize(1080, 2260);

    QObject *root = view.rootObject();
    std::fprintf(stderr, "installed apps visible in SDK target: %d\n", appModel.rowCount());

    auto findByClass = [](QObject *parent, const char *prefix) -> QObject * {
        if (!parent)
            return nullptr;
        for (QObject *o : parent->findChildren<QObject *>()) {
            if (QByteArray(o->metaObject()->className()).startsWith(prefix))
                return o;
        }
        return nullptr;
    };

    QObject *page = nullptr;
    auto eval = [&](const QString &js) {
        if (!page) {
            ++errors;
            return QVariant();
        }
        QObject *inner = findByClass(page, "FolderOverlay");
        QQmlContext *ctx = inner ? QQmlEngine::contextForObject(inner) : view.engine()->contextForObject(page);
        QQmlExpression expr(ctx, page, js);
        const QVariant v = expr.evaluate();
        if (expr.hasError()) {
            ++errors;
            std::fprintf(stderr, "ERROR evaluating %s: %s\n", qPrintable(js), qPrintable(expr.error().toString()));
        }
        std::fprintf(stderr, "eval: %s -> %s\n", qPrintable(js.left(70)), qPrintable(v.toString()));
        return v;
    };

    struct Step { int at; QString js; };
    const QString settingsUrl = QUrl::fromLocalFile(qmlDir + QStringLiteral("/pages/SettingsPage.qml")).toString();
    const QList<Step> steps = {
        { 1500, QStringLiteral("editMode = true; folderOverlay.open(%1); folderOverlay.opened").arg(folderId) },
        { 2000, QStringLiteral("folderOverlay.apps.length") },
        { 2200, QStringLiteral("folderOverlay.close(); editMode = false; folderOverlay.opened") },
        { 2500, QStringLiteral("addApplications(); pageStack.depth") },
        { 5000, QStringLiteral("pageStack.pop(); 0") },
        { 7000, QStringLiteral("newFolder(); pageStack.depth") },
        { 9500, QStringLiteral("pageStack.pop(); 0") },
        { 11500, QStringLiteral("addToFolder(%1); pageStack.depth").arg(folderId) },
        { 14000, QStringLiteral("pageStack.pop(); 0") },
        { 16000, QStringLiteral("var id = homeModel.entryAt(0,0); "
                                "startDrag({x:0,y:0}, 10, 10, {entryId:id, entryType:'app', name:'x', iconSource:'', folderApps:[], missing:false, entryRow:0, entryColumn:0}); "
                                "var s = grid.mapToItem(null, cellWidth*2 + 10, cellHeight*2 + 10); moveDrag(s.x, s.y); "
                                "hoverRow + ',' + hoverCol") },
        { 16100, QStringLiteral("endDrag(true); dragId") },
        { 16500, QStringLiteral("homeModel.entryAt(2,2) >= 0") },
        { 17000, QStringLiteral("pageStack.push('%1'); pageStack.depth").arg(settingsUrl) },
        { 19500, QStringLiteral("pageStack.depth") },
    };

    QTimer::singleShot(1000, [&]() {
        page = findByClass(root, "HomePage");
        std::fprintf(stderr, "home page found: %s\n", page ? "yes" : "no");
        if (!page)
            ++errors;
    });
    for (const Step &step : steps) {
        const QString js = step.js;
        QTimer::singleShot(step.at, [&, js]() { eval(js); });
    }
    QTimer::singleShot(21000, &app, &QGuiApplication::quit);
    app.exec();

    int qmlWarnings = 0;
    for (const QString &m : messages) {
        if (m.contains(QStringLiteral(".qml")) && !m.contains(QStringLiteral("harbour-bureau.png"))
                && !m.contains(QStringLiteral("IndicatorDown")))
            ++qmlWarnings;
    }
    std::printf("compile errors: %d, QML warnings: %d\n", errors, qmlWarnings);
    return errors == 0 && qmlWarnings == 0 ? 0 : 1;
}
