#ifndef SCOPE_PREVIEW_H_
#define SCOPE_PREVIEW_H_

#include <unity/scopes/PreviewQueryBase.h>

namespace unity {
namespace scopes {
class Result;
}
}

namespace scope {

class Preview: public unity::scopes::PreviewQueryBase {
public:
    Preview(const unity::scopes::Result &result,
            const unity::scopes::ActionMetadata &metadata);

    ~Preview() = default;

    void cancelled() override;

    void run(unity::scopes::PreviewReplyProxy const& reply) override;
};

}

#endif // SCOPE_PREVIEW_H_
