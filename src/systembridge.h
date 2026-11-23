#ifndef SYSTEMBRIDGE_H
#define SYSTEMBRIDGE_H

#include <QObject>
#include <QStringList>

class SystemBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool partnerSpaceEnabled READ partnerSpaceEnabled NOTIFY partnerSpaceChanged)
    Q_PROPERTY(int otherPartnerApps READ otherPartnerApps NOTIFY partnerSpaceChanged)
    Q_PROPERTY(QString version READ version CONSTANT)

public:
    explicit SystemBridge(QObject *parent = nullptr);

    bool partnerSpaceEnabled() const;
    int otherPartnerApps() const;
    QString version() const;

    Q_INVOKABLE bool launch(const QString &desktopPath);

    Q_INVOKABLE bool enablePartnerSpace();
    Q_INVOKABLE bool disablePartnerSpace();
    Q_INVOKABLE void reloadPartnerSpace();
    Q_INVOKABLE bool restartHomescreen();

    static QString ownDesktopPath();

signals:
    void partnerSpaceChanged();

private:
    QStringList readPartnerApps() const;
    bool writePartnerApps(const QStringList &paths);
    static QString formatDconfArray(const QStringList &paths);
    static QStringList parseDconfArray(const QString &output);
    static bool runProcess(const QString &program, const QStringList &arguments, QByteArray *output = nullptr);

    QStringList m_partnerApps;
};

#endif
