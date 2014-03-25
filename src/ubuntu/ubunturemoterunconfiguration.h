#ifndef UBUNTU_INTERNAL_UBUNTUREMOTERUNCONFIGURATION_H
#define UBUNTU_INTERNAL_UBUNTUREMOTERUNCONFIGURATION_H

#include <remotelinux/abstractremotelinuxrunconfiguration.h>
#include <coreplugin/id.h>

namespace Ubuntu {
namespace Internal {

class UbuntuRemoteRunConfiguration : public RemoteLinux::AbstractRemoteLinuxRunConfiguration
{
    Q_OBJECT

public:
    UbuntuRemoteRunConfiguration(ProjectExplorer::Target *parent);
    UbuntuRemoteRunConfiguration(ProjectExplorer::Target *parent, UbuntuRemoteRunConfiguration *source);

    // AbstractRemoteLinuxRunConfiguration interface
public:
    virtual QString localExecutableFilePath() const;
    virtual QString remoteExecutableFilePath() const;
    virtual QStringList arguments() const;
    virtual QString workingDirectory() const;
    virtual QString alternateRemoteExecutable() const;
    virtual bool useAlternateExecutable() const;
    virtual Utils::Environment environment() const;
    virtual QStringList soLibSearchPaths () const;

    // RunConfiguration interface
    virtual QWidget *createConfigurationWidget();
    virtual bool isEnabled() const;
    virtual QString disabledReason() const;
    virtual bool isConfigured() const;
    virtual bool ensureConfigured(QString *errorMessage);

    // ProjectConfiguration interface
    virtual bool fromMap(const QVariantMap &map);
    virtual QVariantMap toMap() const;

    static Core::Id typeId ();
    void setArguments (const QStringList &args);

protected:
    bool readDesktopFile (QString *errorMessage);

private:
    QString m_appId;
    QString m_desktopFile;
    QString m_localExecutable;
    QString m_remoteExecutable;
    QStringList m_arguments;

};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUREMOTERUNCONFIGURATION_H
