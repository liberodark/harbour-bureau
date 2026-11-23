#include "../src/appmodel.h"
#include "../src/homemodel.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QTemporaryDir>
#include <QTextStream>

#include <cstdio>

static int failures = 0;
static int checks = 0;

#define CHECK(cond) do { ++checks; if (!(cond)) { ++failures; \
    std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)
#define CHECK_EQ(a, b) do { ++checks; if (!((a) == (b))) { ++failures; \
    QTextStream(stderr) << "FAIL " << __FILE__ << ":" << __LINE__ << ": " << #a << " == " << #b \
    << " (got " << QVariant(a).toString() << ", expected " << QVariant(b).toString() << ")\n"; } } while (0)

static QString writeDesktop(const QString &dir, const QString &id, const QString &name, bool noDisplay = false)
{
    const QString path = dir + QLatin1Char('/') + id + QStringLiteral(".desktop");
    QFile f(path);
    f.open(QIODevice::WriteOnly);
    QTextStream s(&f);
    s << "[Desktop Entry]\nType=Application\nName=" << name << "\nExec=/usr/bin/" << id
      << "\nIcon=" << id << "\n";
    if (noDisplay)
        s << "NoDisplay=true\n";
    return path;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTemporaryDir tmp;
    const QString apps = tmp.path() + QStringLiteral("/applications");
    QDir().mkpath(apps);

    const QString a = writeDesktop(apps, QStringLiteral("app-a"), QStringLiteral("Alpha"));
    const QString b = writeDesktop(apps, QStringLiteral("app-b"), QStringLiteral("Bravo"));
    const QString c = writeDesktop(apps, QStringLiteral("app-c"), QStringLiteral("Charlie"));
    const QString d = writeDesktop(apps, QStringLiteral("app-d"), QStringLiteral("Delta"));
    const QString e = writeDesktop(apps, QStringLiteral("app-e"), QStringLiteral("Echo"));
    writeDesktop(apps, QStringLiteral("hidden"), QStringLiteral("Hidden"), true);

    AppModel appModel(QStringList() << apps, nullptr);
    CHECK_EQ(appModel.rowCount(), 5);
    CHECK(appModel.isInstalled(tmp.path() + QStringLiteral("/applications/hidden.desktop")));
    CHECK_EQ(appModel.infoForPath(b).name, QStringLiteral("Bravo"));
    CHECK_EQ(appModel.infoForPath(b).iconSource, QStringLiteral("image://theme/app-b"));
    CHECK(appModel.infoForPath(apps + QStringLiteral("/nope.desktop")).missing);
    appModel.setFilterText(QStringLiteral("ar"));
    CHECK_EQ(appModel.rowCount(), 1);
    appModel.setFilterText(QString());

    const QString store = tmp.path() + QStringLiteral("/data/layout.json");
    HomeModel home(&appModel, store);
    CHECK(!home.load());

    CHECK_EQ(home.addApps(QStringList() << a << b << c << a), 3);
    CHECK_EQ(home.count(), 3);
    const int idA = home.entryAt(0, 0);
    const int idB = home.entryAt(0, 1);
    const int idC = home.entryAt(0, 2);
    CHECK(idA > 0 && idB > 0 && idC > 0);
    CHECK_EQ(home.data(home.index(0), HomeModel::NameRole).toString(), QStringLiteral("Alpha"));

    CHECK_EQ(home.moveEntry(idC, 3, 2), QStringLiteral("moved"));
    CHECK_EQ(home.entryAt(3, 2), idC);
    CHECK_EQ(home.entryAt(0, 2), -1);
    CHECK_EQ(home.usedRows(), 4);
    CHECK_EQ(home.moveEntry(idC, 3, 2), QStringLiteral("none"));
    CHECK_EQ(home.moveEntry(idC, 5, 99), QStringLiteral("moved"));
    CHECK_EQ(home.entryAt(5, home.columns() - 1), idC);

    CHECK_EQ(home.addApps(QStringList() << d), 1);
    CHECK_EQ(home.entryAt(0, 2), home.entryAt(0, 2));
    CHECK(home.entryAt(0, 2) > 0);
    const int idD = home.entryAt(0, 2);

    CHECK_EQ(home.moveEntry(idB, 0, 0), QStringLiteral("folder"));
    const int folder = home.entryAt(0, 0);
    CHECK_EQ(folder, idA);
    CHECK(home.folderExists(folder));
    CHECK_EQ(home.folderPaths(folder), QStringList() << a << b);
    CHECK_EQ(home.entryAt(0, 1), -1);
    CHECK_EQ(home.folderName(folder), QStringLiteral("Dossier"));
    CHECK(home.containsApp(b));

    CHECK_EQ(home.moveEntry(idD, 0, 0), QStringLiteral("added"));
    CHECK_EQ(home.folderPaths(folder).size(), 3);
    CHECK_EQ(home.count(), 2);

    CHECK_EQ(home.moveEntry(folder, 5, home.columns() - 1), QStringLiteral("swapped"));
    CHECK_EQ(home.entryAt(5, home.columns() - 1), folder);
    CHECK_EQ(home.entryAt(0, 0), idC);

    home.renameFolder(folder, QStringLiteral("  Jeux  "));
    CHECK_EQ(home.folderName(folder), QStringLiteral("Jeux"));
    home.renameFolder(folder, QStringLiteral("   "));
    CHECK_EQ(home.folderName(folder), QStringLiteral("Jeux"));

    home.moveOutOfFolder(folder, d);
    CHECK_EQ(home.folderPaths(folder), QStringList() << a << b);
    CHECK(home.containsApp(d));
    CHECK(home.entryAt(0, 1) > 0);

    home.removeFromFolder(folder, a);
    CHECK(!home.folderExists(folder));
    CHECK(!home.containsApp(a));
    CHECK_EQ(home.data(home.index(home.count() - 1), HomeModel::EntryTypeRole).toString(), QStringLiteral("app"));
    CHECK(home.containsApp(b));

    const int f2 = home.createFolder(QStringLiteral("Travail"), QStringList() << b << e << e);
    CHECK(home.folderExists(f2));
    CHECK_EQ(home.folderPaths(f2), QStringList() << b << e);
    CHECK_EQ(home.allApps().count(b), 1);

    const int f3 = home.createFolder(QString(), QStringList() << a << c);
    CHECK_EQ(home.folderName(f3), QStringLiteral("Dossier"));
    home.addToFolder(f2, QStringList() << a);
    CHECK_EQ(home.folderPaths(f2), QStringList() << b << e << a);
    CHECK(!home.folderExists(f3));
    CHECK_EQ(home.allApps().count(c), 1);
    CHECK_EQ(home.allApps().count(a), 1);

    const int f4 = home.createFolder(QString(), QStringList());
    const int f5 = home.createFolder(QString(), QStringList());
    CHECK(home.folderName(f4) != home.folderName(f5));
    home.removeEntry(f4);
    home.removeEntry(f5);

    home.setColumns(3);
    CHECK_EQ(home.columns(), 3);
    for (int i = 0; i < home.count(); ++i)
        CHECK(home.data(home.index(i), HomeModel::EntryColumnRole).toInt() < 3);
    home.setColumns(99);
    CHECK_EQ(home.columns(), HomeModel::kMaxColumns);
    home.setColumns(4);

    {
        QSet<QString> cells;
        for (int i = 0; i < home.count(); ++i) {
            const QString key = QString::number(home.data(home.index(i), HomeModel::EntryRowRole).toInt())
                    + QLatin1Char(',') + QString::number(home.data(home.index(i), HomeModel::EntryColumnRole).toInt());
            CHECK(!cells.contains(key));
            cells.insert(key);
        }
    }

    home.setShowLabels(false);
    const QStringList before = home.allApps();
    const int countBefore = home.count();
    HomeModel reloaded(&appModel, store);
    CHECK(reloaded.load());
    CHECK_EQ(reloaded.count(), countBefore);
    CHECK_EQ(reloaded.allApps(), before);
    CHECK_EQ(reloaded.columns(), 4);
    CHECK(!reloaded.showLabels());

    {
        QFile f(store);
        f.open(QIODevice::WriteOnly);
        f.write("{\"columns\":4,\"items\":["
                "{\"type\":\"app\",\"path\":\"" + a.toUtf8() + "\",\"row\":0,\"col\":0},"
                "{\"type\":\"app\",\"path\":\"" + a.toUtf8() + "\",\"row\":0,\"col\":1},"
                "{\"type\":\"app\",\"path\":\"" + b.toUtf8() + "\",\"row\":0,\"col\":0},"
                "{\"type\":\"folder\",\"name\":\"\",\"apps\":[\"" + c.toUtf8() + "\",\"" + b.toUtf8() + "\"],\"row\":0,\"col\":9},"
                "{\"type\":\"weird\"}]}");
    }
    HomeModel repaired(&appModel, store);
    CHECK(repaired.load());
    CHECK_EQ(repaired.count(), 3);
    CHECK_EQ(repaired.allApps().count(a), 1);
    CHECK_EQ(repaired.allApps().count(b), 1);
    CHECK(repaired.entryAt(0, 0) >= 0 && repaired.entryAt(0, 1) >= 0 && repaired.entryAt(0, 2) >= 0);

    QFile::remove(d);
    appModel.refresh();
    {
        bool found = false;
        for (int i = 0; i < home.count(); ++i) {
            if (home.data(home.index(i), HomeModel::DesktopPathRole).toString() == d) {
                CHECK(home.data(home.index(i), HomeModel::MissingRole).toBool());
                found = true;
            }
        }
        CHECK(found);
    }

    home.clearAll();
    CHECK_EQ(home.count(), 0);

    std::printf("%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
