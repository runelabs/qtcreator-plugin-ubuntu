#ifndef UBUNTUCMAKETOOL_H
#define UBUNTUCMAKETOOL_H

#include <cmakeprojectmanager/cmaketool.h>
#include <projectexplorer/kit.h>

namespace Ubuntu {
namespace Internal {

class UbuntuCMakeTool : public CMakeProjectManager::CMakeTool
{
    Q_OBJECT
public:
    explicit UbuntuCMakeTool(QObject *parent = 0);
    explicit UbuntuCMakeTool(const Core::Id &id, QObject *parent = 0);

    // ICMakeTool interface
    virtual void addToEnvironment(Utils::Environment &env) const;
    virtual QString displayName ();
    virtual bool isValid() const;

    void setEnvironment (const Utils::Environment &env);

private:
    bool m_hasEnvironment;
    Utils::Environment m_env;

};

class UbuntuCMakeToolFactory : public CMakeProjectManager::ICMakeToolFactory
{
    Q_OBJECT
public:
    virtual bool canCreate (const Core::Id& id) const;
    virtual CMakeProjectManager::ICMakeTool *create (const Core::Id& id);
};

} //namespace Internal
} //namespace Ubuntu

#endif // UBUNTUCMAKETOOL_H
