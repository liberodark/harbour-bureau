#include "homemodel.h"
#include "appmodel.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>

HomeModel::HomeModel(AppModel *apps, const QString &storageFile, QObject *parent)
    : QAbstractListModel(parent)
    , m_apps(apps)
    , m_storageFile(storageFile)
{
    if (m_apps)
        connect(m_apps, &AppModel::appsChanged, this, &HomeModel::refreshAppInfo);
}

int HomeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_entries.size();
}

QVariant HomeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return QVariant();

    const HomeEntry &e = m_entries.at(index.row());
    const bool folder = e.type == HomeEntry::Folder;

    switch (role) {
    case EntryIdRole:
        return e.id;
    case EntryTypeRole:
        return folder ? QStringLiteral("folder") : QStringLiteral("app");
    case EntryRowRole:
        return e.row;
    case EntryColumnRole:
        return e.col;
    case FolderAppsRole:
        return folder ? folderAppsList(e) : QVariantList();
    case FolderCountRole:
        return folder ? e.apps.size() : 0;
    default:
        break;
    }

    if (folder) {
        switch (role) {
        case NameRole:
            return e.name;
        case IconSourceRole:
            return QString();
        case DesktopPathRole:
            return QString();
        case MissingRole:
        case IsAndroidRole:
            return false;
        default:
            return QVariant();
        }
    }

    const AppInfo info = m_apps ? m_apps->infoForPath(e.desktopPath) : AppInfo();
    switch (role) {
    case NameRole:
        return info.name.isEmpty() ? QFileInfo(e.desktopPath).completeBaseName() : info.name;
    case IconSourceRole:
        return info.iconSource;
    case DesktopPathRole:
        return e.desktopPath;
    case MissingRole:
        return info.missing;
    case IsAndroidRole:
        return info.isAndroid;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> HomeModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles.insert(EntryIdRole, "entryId");
    roles.insert(EntryTypeRole, "entryType");
    roles.insert(NameRole, "name");
    roles.insert(IconSourceRole, "iconSource");
    roles.insert(DesktopPathRole, "desktopPath");
    roles.insert(EntryRowRole, "entryRow");
    roles.insert(EntryColumnRole, "entryColumn");
    roles.insert(MissingRole, "missing");
    roles.insert(IsAndroidRole, "isAndroid");
    roles.insert(FolderAppsRole, "folderApps");
    roles.insert(FolderCountRole, "folderCount");
    return roles;
}

QVariantList HomeModel::folderAppsList(const HomeEntry &entry) const
{
    QVariantList list;
    for (const QString &path : entry.apps) {
        const AppInfo info = m_apps ? m_apps->infoForPath(path) : AppInfo();
        QVariantMap map;
        map.insert(QStringLiteral("name"), info.name.isEmpty() ? QFileInfo(path).completeBaseName() : info.name);
        map.insert(QStringLiteral("iconSource"), info.iconSource);
        map.insert(QStringLiteral("desktopPath"), path);
        map.insert(QStringLiteral("missing"), info.missing);
        list.append(map);
    }
    return list;
}

void HomeModel::setColumns(int columns)
{
    columns = qBound(int(kMinColumns), columns, int(kMaxColumns));
    if (columns == m_columns)
        return;

    m_columns = columns;

    QList<int> overflow;
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries.at(i).col >= m_columns) {
            m_entries[i].row = -1;
            overflow.append(i);
        }
    }
    for (int i : overflow) {
        int r = 0, c = 0;
        firstFreeCell(&r, &c);
        m_entries[i].row = r;
        m_entries[i].col = c;
    }
    if (!m_entries.isEmpty())
        emit dataChanged(index(0), index(m_entries.size() - 1));

    emit columnsChanged();
    changed();
}

void HomeModel::setShowLabels(bool show)
{
    if (show == m_showLabels)
        return;
    m_showLabels = show;
    emit showLabelsChanged();
    changed();
}

int HomeModel::usedRows() const
{
    int rows = 0;
    for (const HomeEntry &e : m_entries)
        rows = qMax(rows, e.row + 1);
    return rows;
}

int HomeModel::indexOfId(int id) const
{
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries.at(i).id == id)
            return i;
    }
    return -1;
}

int HomeModel::indexOfApp(const QString &path) const
{
    for (int i = 0; i < m_entries.size(); ++i) {
        const HomeEntry &e = m_entries.at(i);
        if (e.type == HomeEntry::App && e.desktopPath == path)
            return i;
    }
    return -1;
}

int HomeModel::folderIndexContaining(const QString &path) const
{
    for (int i = 0; i < m_entries.size(); ++i) {
        const HomeEntry &e = m_entries.at(i);
        if (e.type == HomeEntry::Folder && e.apps.contains(path))
            return i;
    }
    return -1;
}

int HomeModel::entryAt(int row, int col) const
{
    if (row < 0 || col < 0)
        return -1;
    for (const HomeEntry &e : m_entries) {
        if (e.row == row && e.col == col)
            return e.id;
    }
    return -1;
}

bool HomeModel::containsApp(const QString &path) const
{
    return indexOfApp(path) >= 0 || folderIndexContaining(path) >= 0;
}

QStringList HomeModel::allApps() const
{
    QStringList paths;
    for (const HomeEntry &e : m_entries) {
        if (e.type == HomeEntry::App)
            paths.append(e.desktopPath);
        else
            paths.append(e.apps);
    }
    return paths;
}

bool HomeModel::folderExists(int folderId) const
{
    const int i = indexOfId(folderId);
    return i >= 0 && m_entries.at(i).type == HomeEntry::Folder;
}

QString HomeModel::folderName(int folderId) const
{
    const int i = indexOfId(folderId);
    if (i < 0 || m_entries.at(i).type != HomeEntry::Folder)
        return QString();
    return m_entries.at(i).name;
}

QVariantList HomeModel::folderApps(int folderId) const
{
    const int i = indexOfId(folderId);
    if (i < 0 || m_entries.at(i).type != HomeEntry::Folder)
        return QVariantList();
    return folderAppsList(m_entries.at(i));
}

QStringList HomeModel::folderPaths(int folderId) const
{
    const int i = indexOfId(folderId);
    if (i < 0 || m_entries.at(i).type != HomeEntry::Folder)
        return QStringList();
    return m_entries.at(i).apps;
}

void HomeModel::firstFreeCell(int *row, int *col) const
{
    for (int r = 0; ; ++r) {
        for (int c = 0; c < m_columns; ++c) {
            if (entryAt(r, c) < 0) {
                *row = r;
                *col = c;
                return;
            }
        }
    }
}

int HomeModel::appendEntry(HomeEntry entry)
{
    if (entry.id < 0)
        entry.id = m_nextId++;
    const int n = m_entries.size();
    beginInsertRows(QModelIndex(), n, n);
    m_entries.append(entry);
    endInsertRows();
    return entry.id;
}

void HomeModel::removeAt(int index)
{
    if (index < 0 || index >= m_entries.size())
        return;
    beginRemoveRows(QModelIndex(), index, index);
    m_entries.removeAt(index);
    endRemoveRows();
}

void HomeModel::emitRowChanged(int i)
{
    if (i >= 0 && i < m_entries.size())
        emit dataChanged(index(i), index(i));
}

void HomeModel::normalizeFolder(int i)
{
    if (i < 0 || i >= m_entries.size() || m_entries.at(i).type != HomeEntry::Folder)
        return;

    HomeEntry &e = m_entries[i];
    if (e.apps.isEmpty()) {
        removeAt(i);
    } else if (e.apps.size() == 1) {
        e.type = HomeEntry::App;
        e.desktopPath = e.apps.first();
        e.apps.clear();
        e.name.clear();
        emitRowChanged(i);
    } else {
        emitRowChanged(i);
    }
}

void HomeModel::detachApp(const QString &path, int keepFolderId)
{
    const int i = indexOfApp(path);
    if (i >= 0) {
        removeAt(i);
        return;
    }
    for (int f = 0; f < m_entries.size(); ++f) {
        HomeEntry &e = m_entries[f];
        if (e.type == HomeEntry::Folder && e.id != keepFolderId && e.apps.contains(path)) {
            e.apps.removeAll(path);
            normalizeFolder(f);
            return;
        }
    }
}

QString HomeModel::defaultFolderName() const
{
    const QString base = tr("Dossier");
    QSet<QString> used;
    for (const HomeEntry &e : m_entries) {
        if (e.type == HomeEntry::Folder)
            used.insert(e.name);
    }
    if (!used.contains(base))
        return base;
    for (int n = 2; ; ++n) {
        const QString candidate = base + QLatin1Char(' ') + QString::number(n);
        if (!used.contains(candidate))
            return candidate;
    }
}

int HomeModel::addApps(const QStringList &paths)
{
    int added = 0;
    for (const QString &path : paths) {
        if (path.isEmpty() || containsApp(path))
            continue;
        HomeEntry e;
        e.type = HomeEntry::App;
        e.desktopPath = path;
        firstFreeCell(&e.row, &e.col);
        appendEntry(e);
        ++added;
    }
    if (added > 0)
        changed();
    return added;
}

int HomeModel::createFolder(const QString &name, const QStringList &paths)
{
    QStringList clean;
    for (const QString &p : paths) {
        if (!p.isEmpty() && !clean.contains(p))
            clean.append(p);
    }
    for (const QString &p : clean)
        detachApp(p);

    HomeEntry e;
    e.type = HomeEntry::Folder;
    e.name = name.trimmed().isEmpty() ? defaultFolderName() : name.trimmed();
    e.apps = clean;
    firstFreeCell(&e.row, &e.col);
    const int id = appendEntry(e);
    changed();
    return id;
}

void HomeModel::removeEntry(int id)
{
    const int i = indexOfId(id);
    if (i < 0)
        return;
    removeAt(i);
    changed();
}

QString HomeModel::moveEntry(int id, int row, int col)
{
    const int src = indexOfId(id);
    if (src < 0)
        return QStringLiteral("none");

    row = qMax(0, row);
    col = qBound(0, col, m_columns - 1);

    if (m_entries.at(src).row == row && m_entries.at(src).col == col)
        return QStringLiteral("none");

    const int dstId = entryAt(row, col);
    if (dstId < 0) {
        m_entries[src].row = row;
        m_entries[src].col = col;
        emitRowChanged(src);
        changed();
        return QStringLiteral("moved");
    }

    const int dst = indexOfId(dstId);
    const HomeEntry::Type srcType = m_entries.at(src).type;
    const HomeEntry::Type dstType = m_entries.at(dst).type;

    if (srcType == HomeEntry::App && dstType == HomeEntry::App) {
        const QString srcPath = m_entries.at(src).desktopPath;
        const QString name = defaultFolderName();
        HomeEntry &d = m_entries[dst];
        d.type = HomeEntry::Folder;
        d.name = name;
        d.apps = QStringList() << d.desktopPath << srcPath;
        d.desktopPath.clear();
        emitRowChanged(dst);
        removeAt(src);
        changed();
        return QStringLiteral("folder");
    }

    if (srcType == HomeEntry::App && dstType == HomeEntry::Folder) {
        const QString srcPath = m_entries.at(src).desktopPath;
        if (!m_entries.at(dst).apps.contains(srcPath))
            m_entries[dst].apps.append(srcPath);
        emitRowChanged(dst);
        removeAt(src);
        changed();
        return QStringLiteral("added");
    }

    const int oldRow = m_entries.at(src).row;
    const int oldCol = m_entries.at(src).col;
    m_entries[src].row = row;
    m_entries[src].col = col;
    m_entries[dst].row = oldRow;
    m_entries[dst].col = oldCol;
    emitRowChanged(src);
    emitRowChanged(dst);
    changed();
    return QStringLiteral("swapped");
}

void HomeModel::renameFolder(int folderId, const QString &name)
{
    const int i = indexOfId(folderId);
    const QString trimmed = name.trimmed();
    if (i < 0 || m_entries.at(i).type != HomeEntry::Folder || trimmed.isEmpty())
        return;
    if (m_entries.at(i).name == trimmed)
        return;
    m_entries[i].name = trimmed;
    emitRowChanged(i);
    changed();
}

void HomeModel::addToFolder(int folderId, const QStringList &paths)
{
    bool any = false;
    for (const QString &p : paths) {
        if (p.isEmpty())
            continue;
        int fi = indexOfId(folderId);
        if (fi < 0 || m_entries.at(fi).type != HomeEntry::Folder)
            break;
        if (m_entries.at(fi).apps.contains(p))
            continue;
        detachApp(p, folderId);
        fi = indexOfId(folderId);
        if (fi < 0)
            break;
        m_entries[fi].apps.append(p);
        emitRowChanged(fi);
        any = true;
    }
    if (any)
        changed();
}

void HomeModel::moveOutOfFolder(int folderId, const QString &path)
{
    const int fi = indexOfId(folderId);
    if (fi < 0 || m_entries.at(fi).type != HomeEntry::Folder || !m_entries.at(fi).apps.contains(path))
        return;

    m_entries[fi].apps.removeAll(path);

    HomeEntry e;
    e.type = HomeEntry::App;
    e.desktopPath = path;
    firstFreeCell(&e.row, &e.col);
    appendEntry(e);

    normalizeFolder(indexOfId(folderId));
    changed();
}

void HomeModel::removeFromFolder(int folderId, const QString &path)
{
    const int fi = indexOfId(folderId);
    if (fi < 0 || m_entries.at(fi).type != HomeEntry::Folder || !m_entries.at(fi).apps.contains(path))
        return;
    m_entries[fi].apps.removeAll(path);
    normalizeFolder(fi);
    changed();
}

void HomeModel::clearAll()
{
    beginResetModel();
    m_entries.clear();
    endResetModel();
    changed();
}

void HomeModel::refreshAppInfo()
{
    if (!m_entries.isEmpty())
        emit dataChanged(index(0), index(m_entries.size() - 1));
    ++m_revision;
    emit revisionChanged();
}

void HomeModel::changed()
{
    ++m_revision;
    emit revisionChanged();
    if (!m_loading)
        save();
}

bool HomeModel::save() const
{
    if (m_storageFile.isEmpty())
        return false;

    QJsonArray items;
    for (const HomeEntry &e : m_entries) {
        QJsonObject obj;
        obj.insert(QStringLiteral("row"), e.row);
        obj.insert(QStringLiteral("col"), e.col);
        if (e.type == HomeEntry::Folder) {
            obj.insert(QStringLiteral("type"), QStringLiteral("folder"));
            obj.insert(QStringLiteral("name"), e.name);
            obj.insert(QStringLiteral("apps"), QJsonArray::fromStringList(e.apps));
        } else {
            obj.insert(QStringLiteral("type"), QStringLiteral("app"));
            obj.insert(QStringLiteral("path"), e.desktopPath);
        }
        items.append(obj);
    }

    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("columns"), m_columns);
    root.insert(QStringLiteral("showLabels"), m_showLabels);
    root.insert(QStringLiteral("items"), items);

    QDir().mkpath(QFileInfo(m_storageFile).absolutePath());
    QSaveFile file(m_storageFile);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return file.commit();
}

bool HomeModel::load()
{
    QFile file(m_storageFile);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return false;
    const QJsonObject root = doc.object();

    m_loading = true;
    beginResetModel();
    m_entries.clear();
    m_columns = qBound(int(kMinColumns), root.value(QStringLiteral("columns")).toInt(4), int(kMaxColumns));
    m_showLabels = root.value(QStringLiteral("showLabels")).toBool(true);

    QSet<QString> seen;
    QList<int> pending;
    const QJsonArray items = root.value(QStringLiteral("items")).toArray();
    for (const QJsonValue &value : items) {
        const QJsonObject obj = value.toObject();
        HomeEntry e;
        const QString type = obj.value(QStringLiteral("type")).toString();
        if (type == QLatin1String("folder")) {
            e.type = HomeEntry::Folder;
            e.name = obj.value(QStringLiteral("name")).toString();
            if (e.name.trimmed().isEmpty())
                e.name = tr("Dossier");
            for (const QJsonValue &a : obj.value(QStringLiteral("apps")).toArray()) {
                const QString path = a.toString();
                if (path.isEmpty() || seen.contains(path))
                    continue;
                seen.insert(path);
                e.apps.append(path);
            }
        } else if (type == QLatin1String("app")) {
            e.type = HomeEntry::App;
            e.desktopPath = obj.value(QStringLiteral("path")).toString();
            if (e.desktopPath.isEmpty() || seen.contains(e.desktopPath))
                continue;
            seen.insert(e.desktopPath);
        } else {
            continue;
        }

        e.id = m_nextId++;
        const int row = obj.value(QStringLiteral("row")).toInt(-1);
        const int col = obj.value(QStringLiteral("col")).toInt(-1);
        if (row < 0 || col < 0 || col >= m_columns || entryAt(row, col) >= 0) {
            e.row = -1;
            e.col = -1;
            pending.append(m_entries.size());
        } else {
            e.row = row;
            e.col = col;
        }
        m_entries.append(e);
    }

    for (int i : pending) {
        int r = 0, c = 0;
        firstFreeCell(&r, &c);
        m_entries[i].row = r;
        m_entries[i].col = c;
    }

    endResetModel();
    m_loading = false;
    emit columnsChanged();
    emit showLabelsChanged();
    ++m_revision;
    emit revisionChanged();

    if (!pending.isEmpty())
        save();
    return true;
}
