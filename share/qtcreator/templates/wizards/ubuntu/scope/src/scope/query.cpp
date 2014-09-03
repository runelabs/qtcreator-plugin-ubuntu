#include <boost/algorithm/string/trim.hpp>

#include <scope/query.h>

#include <unity/scopes/Annotation.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/QueryBase.h>
#include <unity/scopes/SearchReply.h>

#include <iomanip>
#include <sstream>

namespace sc = unity::scopes;
namespace alg = boost::algorithm;

using namespace std;
using namespace api;
using namespace scope;

const static string WEATHER_TEMPLATE =
        R"(
{
  "schema-version": 1,
  "template": {
    "category-layout": "grid",
    "card-layout": "horizontal",
    "card-size": "small"
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

const static string CITY_TEMPLATE =
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

Query::Query(const sc::CannedQuery &query, const sc::SearchMetadata &metadata,
        Config::Ptr config) :
        sc::SearchQueryBase(query, metadata), client_(config) {
}

void Query::cancelled() {
    client_.cancel();
}

void Query::run(sc::SearchReplyProxy const& reply) {
    try {
        const sc::CannedQuery &query(sc::SearchQueryBase::query());
        string query_string = alg::trim_copy(query.query_string());

        Client::Current current;
        if (query_string.empty()) {
            current = client_.weather("London,uk");
        } else {
            current = client_.weather(query_string);
        }

        stringstream ss(stringstream::in | stringstream::out);
        ss << current.city.name << ", " << current.city.country;

        auto location_cat = reply->register_category("current", ss.str(), "",
                sc::CategoryRenderer(CITY_TEMPLATE));

        {
            sc::CategorisedResult res(location_cat);
            res.set_uri(to_string(current.city.id));

            stringstream ss(stringstream::in | stringstream::out);
            ss << setprecision(3) << current.weather.temp.cur;
            ss << "°C";
            res.set_title(ss.str());

            res.set_art(current.weather.icon);
            res["subtitle"] = current.weather.description;
            res["description"] = "A description of the result";

            if (!reply->push(res)) {
                return;
            }
        }

        Client::Forecast forecast;
        if (query_string.empty()) {
            forecast = client_.forecast_daily("London,uk");
        } else {
            forecast = client_.forecast_daily(query_string);
        }

        auto forecast_cat = reply->register_category("forecast",
                "7 day forecast", "", sc::CategoryRenderer(WEATHER_TEMPLATE));

        for (const auto &weather : forecast.weather) {
            sc::CategorisedResult res(forecast_cat);
            res.set_uri(to_string(weather.id));

            stringstream ss(stringstream::in | stringstream::out);
            ss << setprecision(3) << weather.temp.max;
            ss << "°C to ";
            ss << setprecision(3) << weather.temp.min;
            ss << "°C";
            res.set_title(ss.str());

            res.set_art(weather.icon);
            res["subtitle"] = weather.description;
            res["description"] = "A description of the result";

            if (!reply->push(res)) {
                return;
            }
        }

    } catch (domain_error &e) {
        cerr << e.what() << endl;
        reply->error(current_exception());
    }
}
