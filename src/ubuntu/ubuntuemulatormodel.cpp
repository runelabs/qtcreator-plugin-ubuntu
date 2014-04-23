#include "ubuntuemulatormodel.h"
#include "ubuntuconstants.h"

#include <utils/projectnamevalidatinglineedit.h>

#include <QMutableStringListIterator>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

namespace Ubuntu {
namespace Internal {

/*!
  \brief UbuntuEmulatorModel
  Provides informations about all existing emulators
  \note This will be merged with the devices model when the
        emulator tool can query the serial number of not running
        emulators
 */

UbuntuEmulatorModel::UbuntuEmulatorModel(QObject *parent) :
    QAbstractListModel(parent) ,
    m_process(0),
    m_emulatorInstalled(false) ,
    m_state(UbuntuEmulatorModel::Initial)
{
    m_process = new UbuntuProcess(this);
    connect(m_process,SIGNAL(message(QString)),this,SLOT(onMessage(QString)));
    connect(m_process,SIGNAL(finished(QString,int)),this,SLOT(processFinished(QString,int)));

    checkEmulatorInstalled();
}

bool UbuntuEmulatorModel::emulatorInstalled() const
{
    return m_emulatorInstalled;
}

void UbuntuEmulatorModel::setEmulatorInstalled(bool arg)
{
    if (m_emulatorInstalled != arg) {
        m_emulatorInstalled = arg;
        emit emulatorInstalledChanged(arg);
    }
}

bool UbuntuEmulatorModel::busy() const
{
    return m_busy;
}

void UbuntuEmulatorModel::setBusy(bool arg)
{
    if (m_busy != arg) {
        m_busy = arg;
        emit busyChanged(arg);
    }
}

QString UbuntuEmulatorModel::state() const
{
    switch(m_state) {
        case CheckEmulator: {
            return tr("Checking if emulator tool is installed");
            break;
        }
        case InstallEmulator: {
            return tr("Installing emulator tool");
            break;
        }
        case CreateEmulatorImage: {
            return tr("Creating emulator image");
            break;
        }
        case FindImages: {
            return tr("Searching for emulator images");
            break;
        }
        default:
            return QString();
            break;
    }
}

void UbuntuEmulatorModel::setState(UbuntuEmulatorModel::State newState)
{
    if(m_state != newState) {
        m_state = newState;
        if(m_state == UbuntuEmulatorModel::Initial || m_state == UbuntuEmulatorModel::Idle)
            setBusy(false);
        else
            setBusy(true);

        emit stateChanged(state());
    }
}

int UbuntuEmulatorModel::rowCount(const QModelIndex &parent) const
{
    if(parent.isValid())
        return 0;
    return m_data.size();
}

QVariant UbuntuEmulatorModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()
            || index.parent().isValid()
            || index.row() < 0
            || index.row() > rowCount())
        return QVariant();

    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return m_data[index.row()].name;
        case UbuntuVersionRole:
            return m_data[index.row()].ubuntuVersion;
        case DeviceVersionRole:
            return m_data[index.row()].deviceVersion;
        case ImageVersionRole:
            return m_data[index.row()].imageVersion;
        default:
            break;
    }
    return QVariant();
}

QHash<int, QByteArray> UbuntuEmulatorModel::roleNames() const
{
    QHash<int, QByteArray> rNames = QAbstractListModel::roleNames();
    rNames.insert(UbuntuVersionRole,"ubuntuVersion");
    rNames.insert(DeviceVersionRole,"deviceVersion");
    rNames.insert(ImageVersionRole,"imageVersion");
    return rNames;
}

Qt::ItemFlags UbuntuEmulatorModel::flags(const QModelIndex &index) const
{
    return QAbstractListModel::flags(index);
}

void UbuntuEmulatorModel::clear()
{
    if(rowCount()) {
        beginResetModel();
        m_data.clear();
        endResetModel();
    }
}

void UbuntuEmulatorModel::checkEmulatorInstalled()
{
    setState(CheckEmulator);

    //beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_EMULATOR_INSTALLED));
    m_process->stop();
    QString sEmulatorPackageName = QLatin1String(Ubuntu::Constants::EMULATOR_PACKAGE_NAME);
    m_process->append(QStringList()
                      << QString::fromLatin1(Constants::UBUNTUWIDGETS_LOCAL_PACKAGE_INSTALLED_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(sEmulatorPackageName)
                      << QCoreApplication::applicationDirPath());
    m_process->start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_EMULATOR_INSTALLED));

}

void UbuntuEmulatorModel::findEmulatorImages()
{
    setState(FindImages);

    //beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_SEARCH_IMAGES));
    m_process->stop();
    m_process->append(QStringList()
                      << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_SEARCH_IMAGES_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                      << QCoreApplication::applicationDirPath());
    m_process->start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_SEARCH_IMAGES));
}

void UbuntuEmulatorModel::installEmulator()
{
    if(m_emulatorInstalled)
        return;

    setState(InstallEmulator);

    //beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_INSTALL_EMULATOR_PACKAGE));
    QString sEmulatorPackageName = QLatin1String(Ubuntu::Constants::EMULATOR_PACKAGE_NAME);
    m_process->stop();
    m_process->append(QStringList()
                      << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_INSTALL_EMULATOR_PACKAGE_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(sEmulatorPackageName)
                      << QCoreApplication::applicationDirPath());
    m_process->start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_INSTALL_EMULATOR_PACKAGE));
}

void UbuntuEmulatorModel::createEmulatorImage(const QString &name)
{
    setState(CreateEmulatorImage);

    //beginAction(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_CREATE_EMULATOR));
    m_process->stop();
    QString strEmulatorName = name;
    QString strEmulatorPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    strEmulatorPath += QDir::separator();
    strEmulatorPath += QLatin1String(Constants::DEFAULT_EMULATOR_PATH);
    strEmulatorPath += QDir::separator();
    m_process->append(QStringList()
                      << QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_CREATE_EMULATOR_SCRIPT)
                         .arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH).arg(strEmulatorPath).arg(strEmulatorName)
                      << QCoreApplication::applicationDirPath());
    m_process->start(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_CREATE_EMULATOR));
}

void UbuntuEmulatorModel::startEmulator(const QString &name)
{
    QStringList args = QStringList() << name;
    QProcess::startDetached(QString::fromLatin1(Constants::UBUNTUDEVICESWIDGET_LOCAL_START_EMULATOR_SCRIPT).arg(Ubuntu::Constants::UBUNTU_SCRIPTPATH)
                            ,args
                            ,QCoreApplication::applicationDirPath());
}

QVariant UbuntuEmulatorModel::validateEmulatorName(const QString &name)
{
    QString error;
    bool result = Utils::ProjectNameValidatingLineEdit::validateProjectName(name,&error);
    QVariantMap m;
    m.insert(QStringLiteral("valid"),result);
    m.insert(QStringLiteral("error"),error);
    return QVariant::fromValue(m);
}

void UbuntuEmulatorModel::onMessage(const QString &msg)
{
    if (msg.startsWith(QLatin1String(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_UNABLE_TO_FETCH))) {
        setEmulatorInstalled(false);
    }
    m_reply.append(msg);
}

void UbuntuEmulatorModel::processFinished(const QString &, int)
{
    State lastState = m_state;

    setState(UbuntuEmulatorModel::Idle);
    switch(lastState) {
        case CheckEmulator: {
            QStringList lines = m_reply.trimmed().split(QLatin1String(Constants::LINEFEED));
            foreach(QString line, lines) {
                line = line.trimmed();
                if (line.isEmpty()) {
                    continue;
                }
                if (line.startsWith(QLatin1String(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_LOCAL_NO_EMULATOR_INSTALLED))) {
                    setEmulatorInstalled(false);
                } else {
                    QStringList lineData = line.split(QLatin1String(Constants::SPACE));
                    QString sEmulatorPackageStatus = lineData.takeFirst();
                    //QString sEmulatorPackageName = lineData.takeFirst();
                    //QString sEmulatorPackageVersion = lineData.takeFirst();
                    if (sEmulatorPackageStatus.startsWith(QLatin1String(Constants::INSTALLED))) {
                        setEmulatorInstalled(true);
                        findEmulatorImages();
                    }
                }
            }
            break;
        }
        case InstallEmulator: {
            QStringList lines = m_reply.trimmed().split(QLatin1String(Constants::LINEFEED));
            foreach(QString line, lines) {
                line = line.trimmed();
                if (line.isEmpty()) {
                    continue;
                }
                if (line.startsWith(QLatin1String(Constants::UBUNTUDEVICESWIDGET_ONFINISHED_LOCAL_NO_EMULATOR_INSTALLED))) {
                    setEmulatorInstalled(false);
                    break;
                } else {
                    QStringList lineData = line.split(QLatin1String(Constants::SPACE));
                    QString sEmulatorPackageStatus = lineData.takeFirst();
                    //QString sEmulatorPackageName = lineData.takeFirst();
                    //QString sEmulatorPackageVersion = lineData.takeFirst();
                    if (sEmulatorPackageStatus.startsWith(QLatin1String(Constants::INSTALLED))) {
                        setEmulatorInstalled(true);
                        findEmulatorImages();
                        break;
                    }
                }
            }
            break;
        }
        case CreateEmulatorImage: {
            findEmulatorImages();
            break;
        }
        case FindImages: {
            QStringList lines = m_reply.trimmed().split(QLatin1String(Constants::LINEFEED));
            clear();

            QList<EmulatorItem> items;

            QMutableStringListIterator iter(lines);
            QRegularExpression regexName   (QStringLiteral("^(\\w+)"));
            QRegularExpression regexUbuntu (QStringLiteral("ubuntu=([0-9]+)"));
            QRegularExpression regexDevice (QStringLiteral("device=([0-9]+)"));
            QRegularExpression regexVersion(QStringLiteral("version=([0-9]+)"));
            while (iter.hasNext()) {
                QString line = iter.next();
                if(line.isEmpty()) {
                    iter.remove();
                    continue;
                }

                qDebug()<<"Handling emulator: "<<line;
                QRegularExpressionMatch mName = regexName.match(line);
                QRegularExpressionMatch mUbu  = regexUbuntu.match(line);
                QRegularExpressionMatch mDev  = regexDevice.match(line);
                QRegularExpressionMatch mVer  = regexVersion.match(line);

                if(!mName.hasMatch())
                    continue;

                EmulatorItem item;
                item.name = mName.captured(1);

                if(mUbu.hasMatch())
                    item.ubuntuVersion = mUbu.captured(1);
                else
                    item.ubuntuVersion = tr("unknown");

                if(mDev.hasMatch())
                    item.deviceVersion = mDev.captured(1);
                else
                    item.deviceVersion = tr("unknown");

                if(mVer.hasMatch())
                    item.imageVersion = mVer.captured(1);
                else
                    item.imageVersion = tr("unknown");

                items.append(item);
            }

            if(items.count()) {
                beginResetModel();
                m_data = items;
                endResetModel();
            }
            break;
        }
        default:
            break;
    }
    m_reply.clear();
}

} // namespace Internal
} // namespace Ubuntu
