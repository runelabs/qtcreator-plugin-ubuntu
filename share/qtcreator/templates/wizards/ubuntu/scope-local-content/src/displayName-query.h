#include <unity/scopes/SearchQueryBase.h>
#include <unity/scopes/ReplyProxyFwd.h>

class %DISPLAYNAME_CAPITAL%Query : public unity::scopes::SearchQueryBase
{
public:
    %DISPLAYNAME_CAPITAL%Query(std::string const& query);
    ~%DISPLAYNAME_CAPITAL%Query();
    virtual void cancelled() override;

    virtual void run(unity::scopes::SearchReplyProxy const& reply) override;

private:
    std::string query_;
};
