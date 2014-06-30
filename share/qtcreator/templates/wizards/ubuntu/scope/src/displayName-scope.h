#ifndef DEMOSCOPE_H
#define DEMOSCOPE_H

#include <unity/scopes/ScopeBase.h>
#include <unity/scopes/QueryBase.h>
#include <unity/scopes/ReplyProxyFwd.h>
#include <unity/scopes/QueryBase.h>
#include <unity/scopes/PreviewQueryBase.h>

class %ProjectName:c%Scope : public unity::scopes::ScopeBase
{
public:
    virtual int start(std::string const&, unity::scopes::RegistryProxy const&) override;

    virtual void stop() override;

    unity::scopes::PreviewQueryBase::UPtr preview(const unity::scopes::Result&,
            const unity::scopes::ActionMetadata&) override;

    virtual unity::scopes::SearchQueryBase::UPtr search(unity::scopes::CannedQuery const& q,
            unity::scopes::SearchMetadata const&) override;
};

#endif
