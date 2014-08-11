#ifndef DEMOSCOPE_H
#define DEMOSCOPE_H

#include <unity/scopes/ScopeBase.h>
#include <unity/scopes/QueryBase.h>
#include <unity/scopes/ReplyProxyFwd.h>
#include <unity/scopes/QueryBase.h>
#include <unity/scopes/PreviewQueryBase.h>

class %ClickHookName:s%Scope : public unity::scopes::ScopeBase
{
public:
    virtual void start(std::string const&) override;

    virtual void stop() override;

    unity::scopes::PreviewQueryBase::UPtr preview(const unity::scopes::Result& result,
            unity::scopes::ActionMetadata const& metadata) override;

    virtual unity::scopes::SearchQueryBase::UPtr search(unity::scopes::CannedQuery const& q,
            unity::scopes::SearchMetadata const& metadata) override;
};

#endif
