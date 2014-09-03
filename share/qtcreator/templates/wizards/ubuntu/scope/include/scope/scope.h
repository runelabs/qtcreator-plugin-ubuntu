#ifndef SCOPE_SCOPE_H_
#define SCOPE_SCOPE_H_

#include <api/config.h>

#include <unity/scopes/ScopeBase.h>
#include <unity/scopes/QueryBase.h>
#include <unity/scopes/ReplyProxyFwd.h>
#include <unity/scopes/QueryBase.h>
#include <unity/scopes/PreviewQueryBase.h>

namespace scope {

class Scope: public unity::scopes::ScopeBase {
public:
    void start(std::string const&) override;

    void stop() override;

    unity::scopes::PreviewQueryBase::UPtr preview(const unity::scopes::Result&,
            const unity::scopes::ActionMetadata&) override;

    virtual unity::scopes::SearchQueryBase::UPtr search(
            unity::scopes::CannedQuery const& q,
            unity::scopes::SearchMetadata const&) override;

protected:
    api::Config::Ptr config_;
};

}

#endif // SCOPE_SCOPE_H_
