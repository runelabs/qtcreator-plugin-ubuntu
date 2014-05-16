#include "%DISPLAYNAME%-query.h"
#include <unity/scopes/Annotation.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/QueryBase.h>
#include <unity/scopes/SearchReply.h>

using namespace unity::scopes;

%DISPLAYNAME_CAPITAL%Query::%DISPLAYNAME_CAPITAL%Query(std::string const& query) :
query_(query)
{
}

%DISPLAYNAME_CAPITAL%Query::~%DISPLAYNAME_CAPITAL%Query()
{
}

void %DISPLAYNAME_CAPITAL%Query::cancelled()
{
}

void %DISPLAYNAME_CAPITAL%Query::run(unity::scopes::SearchReplyProxy const& reply)
{
    CategoryRenderer rdr;
    auto cat = reply->register_category("cat1", "Category 1", "", rdr);
    CategorisedResult res(cat);
    res.set_uri("uri");
    res.set_title("scope-A: result 1 for query \"" + query_ + "\"");
    res.set_art("icon");
    res.set_dnd_uri("dnd_uri");
    reply->push(res);

    CannedQuery q("scope-A", query_, "");
    Annotation annotation(Annotation::Type::Link);
    annotation.add_link("More...", q);
    reply->register_annotation(annotation);

}
