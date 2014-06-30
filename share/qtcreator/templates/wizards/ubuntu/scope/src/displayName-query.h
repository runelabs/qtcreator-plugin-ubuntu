#ifndef DEMOQUERY_H
#define DEMOQUERY_H

#include <unity/scopes/SearchQueryBase.h>
#include <unity/scopes/ReplyProxyFwd.h>

class %ProjectName:c%Query : public unity::scopes::SearchQueryBase
{
public:
    %ProjectName:c%Query(std::string const& query);
    ~%ProjectName:c%Query();
    virtual void cancelled() override;

    virtual void run(unity::scopes::SearchReplyProxy const& reply) override;

private:
    std::string query_;
};

#endif
