#include "ubuntucmaketool.h"
#include "ubuntuconstants.h"
#include <QUuid>

namespace Ubuntu {
namespace Internal {

/*!
 * \brief createId
 * Adds a suffix to every tool id, so every kit has its
 * own cmake instance
 */
static Core::Id createId()
{
    Core::Id newId(Constants::UBUNTU_CLICK_CMAKE_TOOL_ID);
    return newId.withSuffix(QUuid::createUuid().toString());
}

UbuntuCMakeTool::UbuntuCMakeTool(QObject *parent)
    : CMakeProjectManager::CMakeTool(parent)
    , m_hasEnvironment(false)
{
    setId(createId());
}

UbuntuCMakeTool::UbuntuCMakeTool(const Core::Id &id, QObject *parent)
    : CMakeProjectManager::CMakeTool(parent)
    , m_hasEnvironment(false)
{
    setId(id);
}

QString UbuntuCMakeTool::displayName()
{
    return tr("Ubuntu Chroot CMake");
}

void UbuntuCMakeTool::addToEnvironment(Utils::Environment &env) const
{
    Utils::Environment::const_iterator i = m_env.constBegin();
    for(;i != m_env.constEnd();i++)
        env.set(i.key(),i.value());
}

bool UbuntuCMakeTool::isValid() const
{
    if(!m_hasEnvironment)
        return false;
    return CMakeTool::isValid();
}

void UbuntuCMakeTool::setEnvironment(const Utils::Environment &env)
{
    m_hasEnvironment = true;
    m_env = env;
    setCMakeExecutable(QString::fromLatin1(Constants::UBUNTU_CLICK_CMAKE_WRAPPER).arg(Constants::UBUNTU_SCRIPTPATH));
}

bool UbuntuCMakeToolFactory::canCreate(const Core::Id &id) const
{
    return (id.toString().startsWith(QLatin1String(Constants::UBUNTU_CLICK_CMAKE_TOOL_ID)));
}

CMakeProjectManager::ICMakeTool *UbuntuCMakeToolFactory::create(const Core::Id &id)
{
    if(!canCreate(id))
        return 0;

    return new UbuntuCMakeTool(id);
}

} //namespace Internal
} //namespace Ubuntu
