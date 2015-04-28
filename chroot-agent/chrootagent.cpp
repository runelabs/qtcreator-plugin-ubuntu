#include "chrootagent.h"
#include "clickchrootagent_adaptor.h"

#include <QCoreApplication>
#include <QFileSystemWatcher>
#include <QDebug>
#include <QProcess>
#include <QRegularExpression>
#include <QDir>
#include <QUuid>

#include <unistd.h>
#include <sys/types.h>

ChrootAgent *ChrootAgent::m_instance = nullptr;

const char UBUNTU_CLICK_CHROOT_BASEPATH[] = "/var/lib/schroot/chroots";
const char UBUNTU_CLICK_TARGETS_REGEX[]   = "^click-(.*)-([A-Za-z0-9]+)$";

ChrootAgent::ChrootAgent(QObject *parent) :
    QObject(parent),
    m_chrootDirWatcher(nullptr)
{
    m_instance = this;
    new ClickChrootAgentAdaptor(this);

    m_sessionPrefix = QString(QStringLiteral("ucca-%1")).arg(getuid());

    //invoke the initialize once we enter the event loop
    QMetaObject::invokeMethod(this,"initialize",Qt::QueuedConnection);
}

void ChrootAgent::initialize()
{
    foreach(const Chroot &ch, listAvailableClickChroots()) {
        m_knownChroots.insert(
                    toInternalName(ch.framework,ch.architecture),
                    ch);
    }

    findExistingChrootSessions();

    foreach(const QString &key, m_knownChroots.keys()) {
        if(!m_knownChroots[key].session.isEmpty())
            continue;

        m_knownChroots[key].session = createClickSession(m_knownChroots[key]);
    }

    if(!m_chrootDirWatcher) {
        m_chrootDirWatcher = new QFileSystemWatcher(QStringList{QStringLiteral("/etc/schroot/chroot.d")},this);
        connect(m_chrootDirWatcher,SIGNAL(directoryChanged(QString)),this,SLOT(directoryChanged()));
    }

    emit chrootListChanged();
    emit sessionListChanged();
}

void ChrootAgent::directoryChanged()
{
    QList<Chroot> chRootOnSys = listAvailableClickChroots();
    QSet<QString> chRootOnSysKeys;

    foreach(const Chroot &ch, chRootOnSys)
        chRootOnSysKeys.insert(toInternalName(ch.framework,ch.architecture));

    bool changed = false;
    foreach(const QString &key, m_knownChroots.keys()) {
        if(!chRootOnSysKeys.contains(key)) {
            qDebug()<<"Removing chroot"<<key<<"from internal List";
            changed = true;
            m_knownChroots.remove(key);
        }
    }

    if(changed)
        emit chrootListChanged();
}

ChrootAgent *ChrootAgent::instance()
{
    return m_instance;
}

QString ChrootAgent::spawnSession(const QString &framework, const QString &architecture)
{
    QString intName = toInternalName(framework,architecture);

    qDebug()<<"Request to spawn session "<<framework<<architecture;

    Chroot *ch = nullptr;
    bool createSession = false;
    if(m_knownChroots.contains(intName)) {
        if(!m_knownChroots[intName].session.isEmpty()) {
            if(validateClickSession(framework,architecture,m_knownChroots[intName].session))
                return m_knownChroots[intName].session;
            else {
                endClickSession(framework,architecture,m_knownChroots[intName].session);
                m_knownChroots[intName].session.clear();
            }
        }

        createSession = true;
        ch = &m_knownChroots[intName];

    } else {
        createSession = true;
        Chroot newChroot;
        if(!chrootFromPath(QString::fromLatin1("click-%1-%2"),&newChroot))
            return QString();
        m_knownChroots.insert(intName,newChroot);
        ch = &m_knownChroots[intName];

        QTimer::singleShot(0,this,SIGNAL(chrootListChanged()));
    }

    if(createSession) {
        ch->session = createClickSession(*ch);
        QTimer::singleShot(0,this,SIGNAL(sessionListChanged()));
        return ch->session;
    }
    return QString();
}

bool ChrootAgent::releaseSession(const QString &framework, const QString &architecture)
{
    qDebug()<<"Request to release session "<<framework<<architecture;

    QString intName = toInternalName(framework,architecture);
    if(!m_knownChroots.contains(intName))
        return true;

    bool s = endClickSession(m_knownChroots[intName]);
    emit sessionListChanged();

    return s;
}

QStringList ChrootAgent::sessionInfo() const
{
    QStringList info;
    for(auto i = m_knownChroots.constBegin(); i != m_knownChroots.constEnd(); i++) {
        const Chroot &chroot = i.value();
        if(chroot.session.isEmpty())
            continue;

        QString desc = QString(QStringLiteral("Architecture: %1, Framework: %2, Session: %3"))
                .arg(chroot.architecture)
                .arg(chroot.framework)
                .arg(chroot.session);
        info.append(desc);
    }
    return info;
}

void ChrootAgent::shutdown()
{
    foreach(const QString &key, m_knownChroots.keys())
        endClickSession(m_knownChroots[key]);

    qApp->exit(0);
}

void ChrootAgent::hangup()
{
    foreach(const QString &key, m_knownChroots.keys())
        endClickSession(m_knownChroots[key]);

    m_knownChroots.clear();
    initialize();
}

/**
 * @brief UbuntuClickTool::listAvailableTargets
 * @return all currently existing chroot targets in the system
 */
QList<ChrootAgent::Chroot> ChrootAgent::listAvailableClickChroots( )
{
    QList<Chroot> items;

    //use the etc dir, config files show only up if we can actually create session
    QDir chrootDir( QStringLiteral("/etc/schroot/chroot.d") );

    //if the dir does not exist there are no available chroots
    if(!chrootDir.exists())
        return items;

    QStringList availableChroots = chrootDir.entryList(QDir::Files | QDir::NoDotAndDotDot,
                                                       QDir::Name  | QDir::Reversed);

    QRegularExpression clickFilter(QString::fromLatin1(UBUNTU_CLICK_TARGETS_REGEX));
    //iterate over all chroots and check if they are click chroots
    foreach (const QString &chroot, availableChroots) {
        QRegularExpressionMatch match = clickFilter.match(chroot);
        if(!match.hasMatch())
            continue;

        Chroot t;
        if(!chrootFromPath(chroot,&t))
            continue;

        items.append(t);
    }
    return items;
}

void ChrootAgent::findExistingChrootSessions()
{
    QStringList args{
        QStringLiteral("--list"),
        QStringLiteral("--all-sessions")
    };

    QProcess proc;
    proc.setProgram(QStringLiteral("schroot"));
    proc.setArguments(args);
    proc.start();
    proc.waitForFinished();

    //session string format
    //session:click-ubuntu-sdk-15.04-armhf-13e7f626-a933-47f1-a501-329749612445
    QRegularExpression exp(QString::fromLatin1("session:click-(ubuntu-sdk-[0-9.]*)-([\\w]*)-(%1-.*)").arg(m_sessionPrefix));
    while(proc.canReadLine()) {
        QString line = QString::fromLocal8Bit(proc.readLine());
        QRegularExpressionMatch match = exp.match(line);
        if(!match.hasMatch())
            continue;

        QString fw   = match.captured(1);
        QString arch = match.captured(2);
        QString internal_name = toInternalName(fw,arch);

        qDebug()<<"Pickung up existing session: "<<fw<<arch<<match.captured(3);

        if(m_knownChroots.contains(internal_name))
            m_knownChroots[internal_name].session = match.captured(3);
        else {
            Chroot ch;
            ch.architecture = arch;
            ch.framework    = fw;
            ch.internalName = internal_name;
            ch.session      = match.captured(3);
            m_knownChroots.insert(internal_name,ch);
        }
    }
}

/*!
 * \brief UbuntuClickTool::targetFromPath
 * returns true if the given path is a click target
 * if it is, \a tg will be initialized with that targets values
 */
bool ChrootAgent::chrootFromPath(const QString &targetPath, ChrootAgent::Chroot *tg) const
{
    QRegularExpression clickFilter(QString::fromLatin1(UBUNTU_CLICK_TARGETS_REGEX));
    QRegularExpressionMatch match = clickFilter.match(targetPath);
    if(!match.hasMatch()) {
        return false;
    }

    Chroot t;
    t.framework    = match.captured(1);
    t.architecture = match.captured(2);
    t.internalName = toInternalName(t.framework,t.architecture);

    //now read informations about the target
    QFile f(QString::fromLatin1("%1/%2")
            .arg(chrootBasePath(t))
            .arg(QLatin1String("/etc/lsb-release")));

    if (!f.exists()) {
        //there is no lsb-release file... the chroot is broken
        return false;

    }

    *tg = t;
    return true;
}

QString ChrootAgent::chrootBasePath(const ChrootAgent::Chroot &target) const
{
    return QString::fromLatin1("%1/click-%2-%3")
            .arg(QLatin1String(UBUNTU_CLICK_CHROOT_BASEPATH))
            .arg(target.framework)
            .arg(target.architecture);
}

QString ChrootAgent::toInternalName(const QString &framework, const QString &arch) const
{
    return  QString::fromLatin1("%1-%2").arg(framework).arg(arch);;
}

QString ChrootAgent::createClickSession(const ChrootAgent::Chroot &ch)
{
    QString name = QString::fromLatin1("%1-%2").arg(m_sessionPrefix).arg(QUuid::createUuid().toString());
    QStringList args = {
        QStringLiteral("chroot"),
        QStringLiteral("-a"),
        ch.architecture,
        QStringLiteral("-f"),
        ch.framework,
        QStringLiteral("begin-session"),
        name
    };

    if(QProcess::execute(QStringLiteral("click"),args) == 0) {
        qDebug()<<"Created session "<<name;
        return name;
    }

    qDebug()<<"Failed to create Session for "<<ch.architecture<<ch.framework;
    return QString();
}

bool ChrootAgent::endClickSession(ChrootAgent::Chroot &ch)
{
    if(endClickSession(ch.framework,ch.architecture,ch.session)) {
        ch.session = QString();
        return true;
    }
    return false;
}

bool ChrootAgent::endClickSession(const QString &framework, const QString &architecture, const QString &sessionName)
{
    if(sessionName.isEmpty())
        return true;

    QStringList args = {
        QStringLiteral("chroot"),
        QStringLiteral("-a"),
        architecture,
        QStringLiteral("-f"),
        framework,
        QStringLiteral("end-session"),
        sessionName
    };

    if(QProcess::execute(QStringLiteral("click"),args) == 0) {
        qDebug()<<"Removed session"<<sessionName<<"for"<<architecture<<framework;
        return true;
    }

    qDebug()<<"Could not remove session for "<<architecture<<framework<<sessionName;
    return false;
}

bool ChrootAgent::validateClickSession(const QString &framework, const QString &architecture, const QString &sessionName)
{
    if(sessionName.isEmpty())
        return false;

    QStringList args = {
        QStringLiteral("chroot"),
        QStringLiteral("-a"),
        architecture,
        QStringLiteral("-f"),
        framework,
        QStringLiteral("run"),
        QStringLiteral("-n"),
        sessionName,
        QStringLiteral("pwd"),
    };

    QProcess process;
    process.setReadChannelMode(QProcess::ForwardedChannels);
    process.setWorkingDirectory(QStringLiteral("/tmp"));
    process.start(QStringLiteral("click"), args);
    if (!process.waitForFinished(-1) || process.error() == QProcess::FailedToStart) {
        qDebug()<<"Could not verify the click session, assuming its broken.";
        return false; //should not happen
    }

    int sucess = process.exitStatus() == QProcess::NormalExit ? process.exitCode() : -1;
    if(sucess == 0) {
        qDebug()<<"Chroot session is valid";
        return true;
    }

    qDebug()<<"Chroot session is broken";
    return false;
}

