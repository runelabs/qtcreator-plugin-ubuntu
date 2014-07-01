#ifndef DEMOPREVIEW_H
#define DEMOPREVIEW_H

#include<unity/scopes/PreviewQueryBase.h>

class %ClickHookName:s%Preview : public unity::scopes::PreviewQueryBase
{
public:
    %ClickHookName:s%Preview(std::string const& uri);
    ~%ClickHookName:s%Preview();

    virtual void cancelled() override;
    virtual void run(unity::scopes::PreviewReplyProxy const& reply) override;

private:
    std::string uri_;
};

#endif
