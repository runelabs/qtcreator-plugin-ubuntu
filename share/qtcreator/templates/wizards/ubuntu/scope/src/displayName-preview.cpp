#include"%ProjectName%-preview.h"

#include<unity/scopes/PreviewWidget.h>
#include<unity/scopes/ColumnLayout.h>
#include<unity/scopes/PreviewReply.h>

using namespace unity::scopes;

%ProjectName:c%Preview::%ProjectName:c%Preview(std::string const& uri) : uri_(uri)
{
}

%ProjectName:c%Preview::~%ProjectName:c%Preview()
{
}

void %ProjectName:c%Preview::cancelled()
{
}

void %ProjectName:c%Preview::run(unity::scopes::PreviewReplyProxy const& reply)
{
    PreviewWidgetList widgets;
    widgets.emplace_back(PreviewWidget(R"({"id": "header", "type": "header", "components" : { "title": "title", "subtitle": "author" } })"));
    widgets.emplace_back(PreviewWidget(R"({"id": "img", "type": "image", "components" : { "source": "screenshot-url" } })"));

    PreviewWidget w("img2", "image");
    w.add_attribute_value("zoomable", Variant(false));
    w.add_attribute_mapping("source", "screenshot-url");
    widgets.emplace_back(w);

    ColumnLayout layout1col(1);
    layout1col.add_column({"header", "img", "img2"});

    ColumnLayout layout2col(2);
    layout2col.add_column({"header", "img"});
    layout2col.add_column({"img2"});

    ColumnLayout layout3col(3);
    layout3col.add_column({"header"});
    layout3col.add_column({"img"});
    layout3col.add_column({"img2"});

    reply->register_layout({layout1col, layout2col, layout3col});
    reply->push(widgets);
    reply->push("author", Variant("Foo"));
    reply->push("screenshot-url", Variant("/path/to/image.png"));
}
