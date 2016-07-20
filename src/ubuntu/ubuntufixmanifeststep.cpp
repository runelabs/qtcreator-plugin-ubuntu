#include "ubuntufixmanifeststep.h"
#include "ubuntuconstants.h"

#include <utils/fileutils.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QVariant>
#include <QFileInfo>
#include <QJsonArray>

namespace Ubuntu {
namespace Internal {

/*!
 * \class UbuntuFixManifestStep
 * The UbuntuFixManifestStep can be used to fill in fields into
 * a manifest file in the build process, its used in the fat packaging
 * process
 */

UbuntuFixManifestStep::UbuntuFixManifestStep(ProjectExplorer::BuildStepList *bsl) :
    ProjectExplorer::BuildStep(bsl, Core::Id("UbuntuProjectManager.UbuntuFixManifestStep"))
{
}
QStringList UbuntuFixManifestStep::architectures() const
{
    return m_architectures;
}

void UbuntuFixManifestStep::setArchitectures(const QStringList &architectures)
{
    m_architectures = architectures;
}
QString UbuntuFixManifestStep::packageDir() const
{
    return m_packageDir;
}

void UbuntuFixManifestStep::setPackageDir(const QString &packageDir)
{
    m_packageDir = packageDir;
}

bool UbuntuFixManifestStep::init(QList<const BuildStep *> &earlierSteps)
{
    Q_UNUSED(earlierSteps);
    return true;
}

void UbuntuFixManifestStep::run(QFutureInterface<bool> &fi)
{
    bool result = false;
    if (!m_packageDir.isEmpty() ) {
        Utils::FileName manifestFileName = Utils::FileName::fromString(m_packageDir)
                .appendPath(QStringLiteral("manifest.json"));

        QFile manifestFile(manifestFileName.toString());
        if(Q_UNLIKELY(!manifestFile.open(QIODevice::ReadOnly))) {
            emit addOutput(tr("Can not open manifest file for reading."),ProjectExplorer::BuildStep::ErrorOutput);
            fi.reportFinished(&result);
            return;
        }

        if (m_architectures.isEmpty()) {
            emit addOutput(tr("Can not fix manifest file, no architectures are given."),ProjectExplorer::BuildStep::ErrorOutput);
            fi.reportFinished(&result);
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(manifestFile.readAll());
        QJsonObject rootObj = doc.object();

        manifestFile.close();

        if (m_architectures.size() > 1)
            rootObj[QStringLiteral("architecture")] = QJsonArray::fromStringList(m_architectures);
        else
            rootObj[QStringLiteral("architecture")] = QJsonValue(m_architectures.first());

        doc.setObject(rootObj);

        if(!manifestFile.open(QIODevice::WriteOnly | QIODevice::Truncate)){
            emit addOutput(tr("Can not open manifest file for writing."),ProjectExplorer::BuildStep::ErrorOutput);
            fi.reportFinished(&result);
            return;
        }

        manifestFile.write(doc.toJson());
        manifestFile.close();
    }

    result = true;
    fi.reportFinished(&result);
}

ProjectExplorer::BuildStepConfigWidget *UbuntuFixManifestStep::createConfigWidget()
{
    return new ProjectExplorer::SimpleBuildStepConfigWidget(this);
}



} // namespace Internal
} // namespace Ubuntu
