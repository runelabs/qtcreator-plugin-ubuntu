#ifndef DEMOPREVIEW_H
#define DEMOPREVIEW_H

#include<unity/scopes/PreviewQueryBase.h>

class %ProjectName:c%Preview : public unity::scopes::PreviewQueryBase
{
public:
    %ProjectName:c%Preview(std::string const& uri);
    ~%ProjectName:c%Preview();

    virtual void cancelled() override;
    virtual void run(unity::scopes::PreviewReplyProxy const& reply) override;

private:
    std::string uri_;
};

#endif
