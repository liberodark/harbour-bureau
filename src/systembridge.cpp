#include "systembridge.h"

#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>

#ifndef APP_VERSION
#define APP_VERSION "dev"
#endif

namespace {
const char kPartnerKey[] = "/desktop/lipstick-jolla-home/partnerspace/applications";
const char kLcaTool[] = "/usr/bin/lca-tool";
}

SystemBridge::SystemBridge(QObject *parent)
    : QObject(parent)
{
    m_partnerApps = readPartnerApps();
}

QString SystemBridge::ownDesktopPath()
{
    return QStringLiteral("/usr/share/applications/harbour-bureau.desktop");
}

QString SystemBridge::version() const
{
    return QStringLiteral(APP_VERSION);
}

bool SystemBridge::partnerSpaceEnabled() const
{
    return m_partnerApps.contains(ownDesktopPath());
}

int SystemBridge::otherPartnerApps() const
{
    int n = 0;
    for (const QString &p : m_partnerApps) {
        if (p != ownDesktopPath())
            ++n;
    }
    return n;
}

bool SystemBridge::launch(const QString &desktopPath)
{
    const QFileInfo info(desktopPath);
    if (!info.exists())
        return false;

    return QProcess::startDetached(QString::fromLatin1(kLcaTool),
                                   { QStringLiteral("--triggerdesktop"), info.fileName() });
}

bool SystemBridge::enablePartnerSpace()
{
    return writePartnerApps({ ownDesktopPath() });
}

bool SystemBridge::disablePartnerSpace()
{
    QStringList remaining = m_partnerApps;
    remaining.removeAll(ownDesktopPath());
    return writePartnerApps(remaining);
}

void SystemBridge::reloadPartnerSpace()
{
    const QStringList apps = readPartnerApps();
    if (apps != m_partnerApps) {
        m_partnerApps = apps;
        emit partnerSpaceChanged();
    }
}

bool SystemBridge::restartHomescreen()
{
    return runProcess(QStringLiteral("systemctl"),
                      { QStringLiteral("--user"), QStringLiteral("restart"), QStringLiteral("lipstick") });
}

QStringList SystemBridge::readPartnerApps() const
{
    QByteArray output;
    if (!runProcess(QStringLiteral("dconf"),
                    { QStringLiteral("read"), QString::fromLatin1(kPartnerKey) }, &output)) {
        return QStringList();
    }
    return parseDconfArray(QString::fromUtf8(output));
}

bool SystemBridge::writePartnerApps(const QStringList &paths)
{
    bool ok;
    if (paths.isEmpty()) {
        ok = runProcess(QStringLiteral("dconf"),
                        { QStringLiteral("reset"), QString::fromLatin1(kPartnerKey) });
    } else {
        ok = runProcess(QStringLiteral("dconf"),
                        { QStringLiteral("write"), QString::fromLatin1(kPartnerKey), formatDconfArray(paths) });
    }
    reloadPartnerSpace();
    return ok;
}

QString SystemBridge::formatDconfArray(const QStringList &paths)
{
    if (paths.isEmpty())
        return QStringLiteral("@as []");

    QStringList quoted;
    for (const QString &path : paths)
        quoted.append(QStringLiteral("'") + path + QStringLiteral("'"));
    return QStringLiteral("[") + quoted.join(QStringLiteral(", ")) + QStringLiteral("]");
}

QStringList SystemBridge::parseDconfArray(const QString &output)
{
    const QString trimmed = output.trimmed();
    if (trimmed.isEmpty() || trimmed == QStringLiteral("@as []"))
        return QStringList();

    QStringList paths;
    QRegularExpression re(QStringLiteral("'([^']*)'"));
    auto it = re.globalMatch(trimmed);
    while (it.hasNext()) {
        const QString path = it.next().captured(1);
        if (!path.isEmpty())
            paths.append(path);
    }
    return paths;
}

bool SystemBridge::runProcess(const QString &program, const QStringList &arguments, QByteArray *output)
{
    QProcess process;
    process.start(program, arguments);
    if (!process.waitForFinished(30000))
        return false;
    if (output)
        *output = process.readAllStandardOutput();
    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}
