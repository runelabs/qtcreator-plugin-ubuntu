#ifndef DEMOPREVIEW_H
#define DEMOPREVIEW_H

#include<unity/scopes/PreviewQueryBase.h>

class %DISPLAYNAME_CAPITAL%Preview : public unity::scopes::PreviewQueryBase
{
public:
    %DISPLAYNAME_CAPITAL%Preview(std::string const& uri);
    ~%DISPLAYNAME_CAPITAL%Preview();

    virtual void cancelled() override;
    virtual void run(unity::scopes::PreviewReplyProxy const& reply) override;

private:
    std::string uri_;
};

#endif
