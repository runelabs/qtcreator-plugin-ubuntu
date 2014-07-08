#ifndef DEMOPREVIEW_H
#define DEMOPREVIEW_H

#include<unity/scopes/PreviewQueryBase.h>
#include<unity/scopes/Result.h>
#include<unity/scopes/ActionMetadata.h>

class %ClickHookName:s%Preview : public unity::scopes::PreviewQueryBase
{
public:
    %ClickHookName:s%Preview(unity::scopes::Result const& result, unity::scopes::ActionMetadata const& metadata);
    ~%ClickHookName:s%Preview();

    virtual void cancelled() override;
    virtual void run(unity::scopes::PreviewReplyProxy const& reply) override;
};

#endif
