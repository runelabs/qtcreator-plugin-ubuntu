#include <scope/preview.h>

#include <unity/scopes/ColumnLayout.h>
#include <unity/scopes/PreviewWidget.h>
#include <unity/scopes/PreviewReply.h>
#include <unity/scopes/Result.h>
#include <unity/scopes/VariantBuilder.h>

#include <iostream>

namespace sc = unity::scopes;

using namespace std;
using namespace scope;

Preview::Preview(const sc::Result &result, const sc::ActionMetadata &metadata) :
        sc::PreviewQueryBase(result, metadata) {
}

void Preview::cancelled() {
}

void Preview::run(sc::PreviewReplyProxy const& reply) {
    sc::ColumnLayout layout1col(1), layout2col(2), layout3col(3);
    layout1col.add_column( { "image", "header", "summary" });

    layout2col.add_column( { "image" });
    layout2col.add_column( { "header", "summary" });

    layout3col.add_column( { "image" });
    layout3col.add_column( { "header", "summary" });
    layout3col.add_column( { });

    reply->register_layout( { layout1col, layout2col, layout3col });

    sc::PreviewWidget header("header", "header");
    header.add_attribute_mapping("title", "title");
    header.add_attribute_mapping("subtitle", "subtitle");

    sc::PreviewWidget image("image", "image");
    image.add_attribute_mapping("source", "art");

    sc::PreviewWidget description("summary", "text");
    description.add_attribute_mapping("text", "description");

    reply->push( { image, header, description });
}
