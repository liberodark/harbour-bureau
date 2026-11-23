#ifndef APPMODEL_H
#define APPMODEL_H

#include "appinfo.h"

#include <QAbstractListModel>
#include <QFileSystemWatcher>
#include <QHash>
#include <QStringList>
#include <QTimer>

class AppModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        DesktopPathRole,
        IconSourceRole,
        IsAndroidRole,
    };

    explicit AppModel(QObject *parent = nullptr);
    AppModel(const QStringList &directories, QObject *parent);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString filterText() const;
    void setFilterText(const QString &text);

    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool isInstalled(const QString &path) const;

    AppInfo infoForPath(const QString &path) const;

signals:
    void filterTextChanged();
    void countChanged();
    void appsChanged();

private:
    void init();
    void scanDirectory(const QString &dirPath);
    bool passesFilter(const AppInfo &info) const;
    void rebuildVisible();
    void updateWatches();

    QStringList m_directories;
    QList<AppInfo> m_allApps;
    QList<AppInfo> m_visibleApps;
    QHash<QString, AppInfo> m_byPath;
    QString m_filterText;
    QFileSystemWatcher *m_watcher = nullptr;
    QTimer m_refreshTimer;
};

#endif
