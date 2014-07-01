#include "%ClickHookName:l%-scope.h"
#include "%ClickHookName:l%-query.h"
#include "%ClickHookName:l%-preview.h"
#include <unity-scopes.h>

using namespace unity::scopes;

int %ClickHookName:s%Scope::start(std::string const&, unity::scopes::RegistryProxy const&)
{
    return VERSION;
}

void %ClickHookName:s%Scope::stop()
{
}

SearchQueryBase::UPtr %ClickHookName:s%Scope::search(unity::scopes::CannedQuery const &q,
        unity::scopes::SearchMetadata const&)
{
    unity::scopes::SearchQueryBase::UPtr query(new %ClickHookName:s%Query(q.query_string()));
    return query;
}

PreviewQueryBase::UPtr %ClickHookName:s%Scope::preview(Result const& result, ActionMetadata const& /*metadata*/) {
    unity::scopes::PreviewQueryBase::UPtr preview(new %ClickHookName:s%Preview(result.uri()));
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
        return new %ClickHookName:s%Scope();
    }

    EXPORT
    void
    // cppcheck-suppress unusedFunction
    UNITY_SCOPE_DESTROY_FUNCTION(unity::scopes::ScopeBase* scope_base)
    {
        delete scope_base;
    }

}
