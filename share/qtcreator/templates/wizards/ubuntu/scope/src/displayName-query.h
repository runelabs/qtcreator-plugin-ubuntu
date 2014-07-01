#ifndef DEMOQUERY_H
#define DEMOQUERY_H

#include <unity/scopes/SearchQueryBase.h>
#include <unity/scopes/ReplyProxyFwd.h>

class %ClickHookName:s%Query : public unity::scopes::SearchQueryBase
{
public:
    %ClickHookName:s%Query(unity::scopes::CannedQuery const& query, unity::scopes::SearchMetadata const& metadata);
    ~%ClickHookName:s%Query();
    virtual void cancelled() override;

    virtual void run(unity::scopes::SearchReplyProxy const& reply) override;
};

#endif
