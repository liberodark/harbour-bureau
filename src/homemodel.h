#ifndef HOMEMODEL_H
#define HOMEMODEL_H

#include <QAbstractListModel>
#include <QStringList>
#include <QVariantList>

class AppModel;

struct HomeEntry
{
    enum Type { App = 0, Folder = 1 };

    int id = -1;
    Type type = App;
    QString desktopPath;
    QString name;
    QStringList apps;
    int row = 0;
    int col = 0;
};

class HomeModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int columns READ columns WRITE setColumns NOTIFY columnsChanged)
    Q_PROPERTY(bool showLabels READ showLabels WRITE setShowLabels NOTIFY showLabelsChanged)
    Q_PROPERTY(int usedRows READ usedRows NOTIFY revisionChanged)
    Q_PROPERTY(int count READ count NOTIFY revisionChanged)
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)

public:
    enum Roles {
        EntryIdRole = Qt::UserRole + 1,
        EntryTypeRole,
        NameRole,
        IconSourceRole,
        DesktopPathRole,
        EntryRowRole,
        EntryColumnRole,
        MissingRole,
        IsAndroidRole,
        FolderAppsRole,
        FolderCountRole,
    };

    static const int kMinColumns = 3;
    static const int kMaxColumns = 6;

    HomeModel(AppModel *apps, const QString &storageFile, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int columns() const { return m_columns; }
    void setColumns(int columns);
    bool showLabels() const { return m_showLabels; }
    void setShowLabels(bool show);
    int usedRows() const;
    int count() const { return m_entries.size(); }
    int revision() const { return m_revision; }

    Q_INVOKABLE int entryAt(int row, int col) const;
    Q_INVOKABLE bool containsApp(const QString &path) const;
    Q_INVOKABLE QStringList allApps() const;
    Q_INVOKABLE bool folderExists(int folderId) const;
    Q_INVOKABLE QString folderName(int folderId) const;
    Q_INVOKABLE QVariantList folderApps(int folderId) const;
    Q_INVOKABLE QStringList folderPaths(int folderId) const;

    Q_INVOKABLE int addApps(const QStringList &paths);
    Q_INVOKABLE int createFolder(const QString &name, const QStringList &paths);
    Q_INVOKABLE void removeEntry(int id);
    Q_INVOKABLE QString moveEntry(int id, int row, int col);
    Q_INVOKABLE void renameFolder(int folderId, const QString &name);
    Q_INVOKABLE void addToFolder(int folderId, const QStringList &paths);
    Q_INVOKABLE void moveOutOfFolder(int folderId, const QString &path);
    Q_INVOKABLE void removeFromFolder(int folderId, const QString &path);
    Q_INVOKABLE void clearAll();

    bool load();
    bool save() const;

public slots:
    void refreshAppInfo();

signals:
    void columnsChanged();
    void showLabelsChanged();
    void revisionChanged();

private:
    int indexOfId(int id) const;
    int indexOfApp(const QString &path) const;
    int folderIndexContaining(const QString &path) const;
    void firstFreeCell(int *row, int *col) const;
    int appendEntry(HomeEntry entry);
    void removeAt(int index);
    void emitRowChanged(int index);
    void detachApp(const QString &path, int keepFolderId = -1);
    void normalizeFolder(int index);
    QString defaultFolderName() const;
    void changed();
    QVariantList folderAppsList(const HomeEntry &entry) const;

    AppModel *m_apps;
    QString m_storageFile;
    QList<HomeEntry> m_entries;
    int m_columns = 4;
    bool m_showLabels = true;
    int m_nextId = 1;
    int m_revision = 0;
    bool m_loading = false;
};

#endif
