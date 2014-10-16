#ifndef UBUNTU_INTERNAL_UBUNTUTESTCONTROL_H
#define UBUNTU_INTERNAL_UBUNTUTESTCONTROL_H

#include <QObject>
#include <projectexplorer/buildmanager.h>

namespace Ubuntu {
namespace Internal {

class UbuntuTestControl : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool lastBuildSuccess READ lastBuildSuccess)
public:
    explicit UbuntuTestControl(QObject *parent = 0);

signals:
    void buildFinished ();
    void lastBuildSuccessChanged(bool arg);

public slots:
    void setLastBuildSuccess(bool arg);
    bool lastBuildSuccess() const;
    void triggerCommand(const QString &command);
    ProjectExplorer::BuildManager *buildManager();

protected:
    bool eventFilter(QObject *, QEvent *event);
private:
    bool m_lastBuildSuccess;
};

} // namespace Internal
} // namespace Ubuntu

#endif // UBUNTU_INTERNAL_UBUNTUTESTCONTROL_H
