#include "%ClickHookName:l%-query.h"
#include <unity/scopes/Annotation.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/QueryBase.h>
#include <unity/scopes/SearchReply.h>

using namespace unity::scopes;

namespace
{
const static std::string CATEGORY_TEMPLATE =
        R"(
{
  "schema-version": 1,
  "template": {
    "category-layout": "grid",
    "card-size": "medium"
  },
  "components": {
    "title": "title",
    "art" : {
      "field": "art"
    },
    "subtitle": "subtitle"
  }
}
)";
}

%ClickHookName:s%Query::%ClickHookName:s%Query(CannedQuery const& query, SearchMetadata const& metadata) :
 SearchQueryBase(query, metadata)
{
}

%ClickHookName:s%Query::~%ClickHookName:s%Query()
{
}

void %ClickHookName:s%Query::cancelled()
{
}

void %ClickHookName:s%Query::run(unity::scopes::SearchReplyProxy const& reply)
{
    CategoryRenderer rdr(CATEGORY_TEMPLATE);
    auto cat = reply->register_category("cat1", "Category 1", "", rdr);
    CategorisedResult res(cat);
    res.set_uri("uri");
    res.set_title("scope-A: result 1 for query \"" + query().query_string() + "\"");
    res.set_art("http://design.ubuntu.com/wp-content/uploads/ubuntu-logo32.png");
    res.set_dnd_uri("dnd_uri");
    reply->push(res);
}
