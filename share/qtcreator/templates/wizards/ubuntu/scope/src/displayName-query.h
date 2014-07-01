#ifndef DEMOQUERY_H
#define DEMOQUERY_H

#include <unity/scopes/SearchQueryBase.h>
#include <unity/scopes/ReplyProxyFwd.h>

class %ClickHookName:s%Query : public unity::scopes::SearchQueryBase
{
public:
    %ClickHookName:s%Query(std::string const& query);
    ~%ClickHookName:s%Query();
    virtual void cancelled() override;

    virtual void run(unity::scopes::SearchReplyProxy const& reply) override;

private:
    std::string query_;
};

#endif
