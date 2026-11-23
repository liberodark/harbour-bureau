#include "desktopparser.h"

#include <MDesktopEntry>

#include <QFileInfo>

QString iconSourceForIcon(const QString &icon)
{
    if (icon.isEmpty())
        return QStringLiteral("image://theme/icon-launcher-default");
    if (icon.startsWith(QLatin1Char('/')))
        return QStringLiteral("file://") + icon;
    if (icon.contains(QStringLiteral("://")))
        return icon;
    return QStringLiteral("image://theme/") + icon;
}

bool parseDesktopFile(const QString &path, AppInfo *info)
{
    if (!info)
        return false;

    MDesktopEntry entry(path);
    if (!entry.isValid())
        return false;
    if (entry.type() != QLatin1String("Application"))
        return false;

    QString name = entry.name();
    if (name.isEmpty())
        name = QFileInfo(path).completeBaseName();

    info->desktopPath = path;
    info->name = name;
    info->iconSource = iconSourceForIcon(entry.icon());
    info->isAndroid = entry.contains(QStringLiteral("Desktop Entry"), QStringLiteral("X-apkd-packageName"))
            || QFileInfo(path).fileName().startsWith(QLatin1String("apkd_launcher_"));
    info->visible = !entry.noDisplay()
            && !entry.hidden()
            && !entry.notShowIn().contains(QStringLiteral("X-MeeGo"));
    info->missing = false;
    return true;
}
