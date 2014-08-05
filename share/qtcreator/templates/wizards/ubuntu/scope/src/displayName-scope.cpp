#include "%ClickHookName:l%-scope.h"
#include "%ClickHookName:l%-query.h"
#include "%ClickHookName:l%-preview.h"
#include <unity-scopes.h>

using namespace unity::scopes;

void %ClickHookName:s%Scope::start(std::string const&)
{
}

void %ClickHookName:s%Scope::stop()
{
}

SearchQueryBase::UPtr %ClickHookName:s%Scope::search(CannedQuery const &q, SearchMetadata const& metadata)
{
    SearchQueryBase::UPtr query(new %ClickHookName:s%Query(q, metadata));
    return query;
}


PreviewQueryBase::UPtr %ClickHookName:s%Scope::preview(Result const& result, ActionMetadata const& metadata) {
    PreviewQueryBase::UPtr preview(new %ClickHookName:s%Preview(result, metadata));
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
