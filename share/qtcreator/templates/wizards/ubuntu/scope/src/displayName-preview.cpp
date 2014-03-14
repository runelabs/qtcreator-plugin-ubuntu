#include"%DISPLAYNAME%-preview.h"

#include<unity/scopes/PreviewWidget.h>
#include<unity/scopes/ColumnLayout.h>
#include<unity/scopes/PreviewReply.h>

using namespace unity::scopes;

%DISPLAYNAME_CAPITAL%Preview::%DISPLAYNAME_CAPITAL%Preview(std::string const& uri) : uri_(uri)
{
}

%DISPLAYNAME_CAPITAL%Preview::~%DISPLAYNAME_CAPITAL%Preview()
{
}

void %DISPLAYNAME_CAPITAL%Preview::cancelled()
{
}

void %DISPLAYNAME_CAPITAL%Preview::run(unity::scopes::PreviewReplyProxy const& reply)
{
    PreviewWidgetList widgets;
    widgets.emplace_back(PreviewWidget(R"({"id": "header", "type": "header", "title": "title", "subtitle": "author", "rating": "rating"})"));
    widgets.emplace_back(PreviewWidget(R"({"id": "img", "type": "image", "art": "screenshot-url"})"));

    PreviewWidget w("img2", "image");
    w.add_attribute_value("zoomable", Variant(false));
    w.add_attribute_mapping("art", "screenshot-url");
    widgets.emplace_back(w);

    ColumnLayout layout1col(1);
    layout1col.add_column({"header", "title"});

    ColumnLayout layout2col(2);
    layout2col.add_column({"header", "title"});
    layout2col.add_column({"author", "rating"});

    ColumnLayout layout3col(3);
    layout3col.add_column({"header", "title"});
    layout3col.add_column({"author"});
    layout3col.add_column({"rating"});

    reply->register_layout({layout1col, layout2col, layout3col});
    reply->push(widgets);
    reply->push("author", Variant("Foo"));
    reply->push("rating", Variant("4 blah"));
}
