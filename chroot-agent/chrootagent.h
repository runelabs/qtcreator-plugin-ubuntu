#ifndef CHROOTAGENT_H
#define CHROOTAGENT_H

#include <qdbusmacros.h>

#include <QObject>
#include <QStringList>
#include <QVariantMap>

class QFileSystemWatcher;

class ChrootAgent : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.ubuntu.sdk.ClickChrootAgent")

    struct Chroot{
        QString internalName;
        QString framework;
        QString architecture;
        QString session;
    };

public:
    explicit ChrootAgent(QObject *parent = 0);

    static ChrootAgent *instance ();

signals:
    void chrootListChanged  ();
    void sessionListChanged ();

public slots:
    QString spawnSession   (const QString &framework, const QString &architecture);
    bool releaseSession (const QString &framework, const QString &architecture);

    QStringList sessionInfo () const;

    Q_NOREPLY void shutdown ();
    void hangup   ();

private slots:
    void initialize ();
    void directoryChanged ();

private:
    void findExistingChrootSessions();
    bool chrootFromPath(const QString &targetPath, ChrootAgent::Chroot *tg) const;
    QString chrootBasePath(const ChrootAgent::Chroot &target) const;
    QString toInternalName (const QString &framework, const QString &arch) const;
    QString createClickSession(const Chroot &ch);
    bool endClickSession(Chroot &ch);
    bool endClickSession(const QString &framework, const QString &architecture, const QString &sessionName);
    bool validateClickSession (const QString &framework, const QString &architecture, const QString &sessionName);

private:
    QMap<QString,Chroot> m_knownChroots;
    QFileSystemWatcher *m_chrootDirWatcher;
    QString             m_sessionPrefix;
    static ChrootAgent *m_instance;


    QList<ChrootAgent::Chroot> listAvailableClickChroots();
};

#endif // CHROOTAGENT_H

