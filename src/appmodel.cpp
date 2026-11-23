#include "appmodel.h"
#include "desktopparser.h"

#include <algorithm>

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

AppModel::AppModel(QObject *parent)
    : QAbstractListModel(parent)
{
    const QString localApps = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QDir().mkpath(localApps);
    m_directories = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    if (!m_directories.contains(QStringLiteral("/usr/share/applications")))
        m_directories.append(QStringLiteral("/usr/share/applications"));

    m_watcher = new QFileSystemWatcher(this);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, [this]() {
        m_refreshTimer.start();
    });
    init();
}

AppModel::AppModel(const QStringList &directories, QObject *parent)
    : QAbstractListModel(parent)
    , m_directories(directories)
{
    init();
}

void AppModel::init()
{
    m_refreshTimer.setSingleShot(true);
    m_refreshTimer.setInterval(1500);
    connect(&m_refreshTimer, &QTimer::timeout, this, &AppModel::refresh);
    refresh();
}

int AppModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_visibleApps.size();
}

QVariant AppModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visibleApps.size())
        return QVariant();

    const AppInfo &info = m_visibleApps.at(index.row());
    switch (role) {
    case NameRole:
        return info.name;
    case DesktopPathRole:
        return info.desktopPath;
    case IconSourceRole:
        return info.iconSource;
    case IsAndroidRole:
        return info.isAndroid;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> AppModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles.insert(NameRole, "name");
    roles.insert(DesktopPathRole, "desktopPath");
    roles.insert(IconSourceRole, "iconSource");
    roles.insert(IsAndroidRole, "isAndroid");
    return roles;
}

QString AppModel::filterText() const
{
    return m_filterText;
}

void AppModel::setFilterText(const QString &text)
{
    if (m_filterText == text)
        return;

    m_filterText = text;
    rebuildVisible();
    emit filterTextChanged();
}

void AppModel::refresh()
{
    beginResetModel();
    m_allApps.clear();
    m_byPath.clear();

    for (const QString &dir : m_directories)
        scanDirectory(dir);

    std::sort(m_allApps.begin(), m_allApps.end(), [](const AppInfo &a, const AppInfo &b) {
        return a.name.localeAwareCompare(b.name) < 0;
    });

    m_visibleApps.clear();
    for (const AppInfo &info : m_allApps) {
        if (passesFilter(info))
            m_visibleApps.append(info);
    }
    endResetModel();

    updateWatches();
    emit countChanged();
    emit appsChanged();
}

void AppModel::updateWatches()
{
    if (!m_watcher)
        return;
    for (const QString &dir : m_directories) {
        if (QFileInfo(dir).isDir() && !m_watcher->directories().contains(dir))
            m_watcher->addPath(dir);
    }
}

void AppModel::scanDirectory(const QString &dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists())
        return;

    const QStringList entries = dir.entryList({ QStringLiteral("*.desktop") }, QDir::Files);
    for (const QString &fileName : entries) {
        const QString path = dir.absoluteFilePath(fileName);
        if (m_byPath.contains(path))
            continue;

        AppInfo info;
        if (!parseDesktopFile(path, &info))
            continue;

        m_byPath.insert(path, info);
        if (info.visible)
            m_allApps.append(info);
    }
}

bool AppModel::passesFilter(const AppInfo &info) const
{
    if (m_filterText.isEmpty())
        return true;
    return info.name.contains(m_filterText, Qt::CaseInsensitive);
}

void AppModel::rebuildVisible()
{
    beginResetModel();
    m_visibleApps.clear();
    for (const AppInfo &info : m_allApps) {
        if (passesFilter(info))
            m_visibleApps.append(info);
    }
    endResetModel();
    emit countChanged();
}

bool AppModel::isInstalled(const QString &path) const
{
    return m_byPath.contains(path);
}

AppInfo AppModel::infoForPath(const QString &path) const
{
    const auto it = m_byPath.constFind(path);
    if (it != m_byPath.constEnd())
        return it.value();

    AppInfo info;
    if (QFileInfo::exists(path) && parseDesktopFile(path, &info))
        return info;

    info.desktopPath = path;
    info.name = QFileInfo(path).completeBaseName();
    info.iconSource = QStringLiteral("image://theme/icon-launcher-default");
    info.missing = true;
    return info;
}
