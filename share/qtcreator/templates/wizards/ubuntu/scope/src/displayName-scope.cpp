#include "%DISPLAYNAME%-scope.h"
#include "%DISPLAYNAME%-query.h"
#include "%DISPLAYNAME%-preview.h"
#include <unity-scopes.h>

using namespace unity::scopes;

int %DISPLAYNAME_CAPITAL%Scope::start(std::string const&, unity::scopes::RegistryProxy const&)
{
    return VERSION;
}

void %DISPLAYNAME_CAPITAL%Scope::stop()
{
}

SearchQueryBase::UPtr %DISPLAYNAME_CAPITAL%Scope::search(unity::scopes::CannedQuery const &q,
        unity::scopes::SearchMetadata const&)
{
    unity::scopes::SearchQueryBase::UPtr query(new %DISPLAYNAME_CAPITAL%Query(q.query_string()));
    return query;
}

PreviewQueryBase::UPtr %DISPLAYNAME_CAPITAL%Scope::preview(Result const& result, ActionMetadata const& /*metadata*/) {
    unity::scopes::PreviewQueryBase::UPtr preview(new %DISPLAYNAME_CAPITAL%Preview(result.uri()));
    return preview;
}

#define EXPORT __attribute__ ((visibility ("default")))

extern "C"
{

    EXPORT
    unity::scopes::ScopeBase*
    // cppcheck-suppress unusedFunction
    UNITY_SCOPE_CREATE_FUNCTION()
    {
        return new %DISPLAYNAME_CAPITAL%Scope();
    }

    EXPORT
    void
    // cppcheck-suppress unusedFunction
    UNITY_SCOPE_DESTROY_FUNCTION(unity::scopes::ScopeBase* scope_base)
    {
        delete scope_base;
    }

}
