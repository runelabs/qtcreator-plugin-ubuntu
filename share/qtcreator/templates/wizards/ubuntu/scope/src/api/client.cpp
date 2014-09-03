#include <api/client.h>

#include <core/net/error.h>
#include <core/net/http/client.h>
#include <core/net/http/content_type.h>
#include <core/net/http/response.h>
#include <json/json.h>

namespace http = core::net::http;
namespace json = Json;
namespace net = core::net;

using namespace api;
using namespace std;

Client::Client(Config::Ptr config) :
        config_(config), cancelled_(false) {
}

void Client::get(const net::Uri::Path &path,
        const net::Uri::QueryParameters &parameters, json::Value &root) {
    // Create a new HTTP client
    auto client = http::make_client();

    // Start building the request configuration
    http::Request::Configuration configuration;

    // Build the URI from its components
    net::Uri uri = net::make_uri(config_->apiroot, path, parameters);
    configuration.uri = client->uri_to_string(uri);

    // Give out a user agent string
    configuration.header.add("User-Agent", config_->user_agent);

    // Build a HTTP request object from our configuration
    auto request = client->head(configuration);

    try {
        // Synchronously make the HTTP request
        // We bind the cancellable callback to #progress_report
        auto response = request->execute(
                bind(&Client::progress_report, this, placeholders::_1));

        // Check that we got a sensible HTTP status code
        if (response.status != http::Status::ok) {
            throw domain_error(root["error"].asString());
        }

        // Parse the JSON from the response
        json::Reader reader;
        reader.parse(response.body, root);

        // Open weather map API error code can either be a string or int
        json::Value cod = root["cod"];
        if ((cod.isString() && cod.asString() != "200")
                || (cod.isUInt() && cod.asUInt() != 200)) {
            throw domain_error(root["message"].asString());
        }
    } catch (net::Error &) {
    }
}

Client::Current Client::weather(const string& query) {
    json::Value root;

    // Build a URI and get the contents.
    // The fist parameter forms the path part of the URI.
    // The second parameter forms the CGI parameters.
    get( { "data", "2.5", "weather" },
            { { "q", query }, { "units", "metric" } }, root);
    // e.g. http://api.openweathermap.org/data/2.5/weather?q=QUERY&units=metric

    Current result;

    // Read out the city we found
    json::Value sys = root["sys"];
    result.city.id = sys["id"].asUInt();
    result.city.name = root["name"].asString();
    result.city.country = sys["country"].asString();

    // Read the weather
    json::Value weather = root["weather"].get(json::ArrayIndex(0),
            json::Value());
    result.weather.id = weather["id"].asUInt();
    result.weather.main = weather["main"].asString();
    result.weather.description = weather["description"].asString();
    result.weather.icon = "http://openweathermap.org/img/w/"
            + weather["icon"].asString() + ".png";

    // Read the temps
    json::Value main = root["main"];
    result.weather.temp.cur = main["temp"].asDouble();
    result.weather.temp.max = main["temp_max"].asDouble();
    result.weather.temp.min = main["temp_min"].asDouble();

    return result;
}

Client::Forecast Client::forecast_daily(const string& query, unsigned int cnt) {
    json::Value root;

    // Build a URI and get the contents
    // The fist parameter forms the path part of the URI.
    // The second parameter forms the CGI parameters.
    get( { "data", "2.5", "forecast", "daily" }, { { "q", query }, { "units",
            "metric" }, { "cnt", to_string(cnt) } }, root);
    // e.g. http://api.openweathermap.org/data/2.5/forecast/daily/?q=QUERY&units=metric&cnt=7

    Forecast result;

    // Read out the city we found
    json::Value city = root["city"];
    result.city.id = city["id"].asUInt();
    result.city.name = city["name"].asString();
    result.city.country = city["country"].asString();

    // Iterate through the weather data
    json::Value list = root["list"];
    for (json::ArrayIndex index = 0; index < list.size(); ++index) {
        json::Value item = list.get(index, json::Value());

        // Extract the first weather item
        json::Value weather_list = item["weather"];
        json::Value weather = weather_list.get(json::ArrayIndex(0),
                json::Value());

        // Extract the temperature data
        json::Value temp = item["temp"];

        // Add a result to the weather list
        result.weather.emplace_back(
                Weather { weather["id"].asUInt(), weather["main"].asString(),
                        weather["description"].asString(),
                        "http://openweathermap.org/img/w/"
                                + weather["icon"].asString() + ".png", Temp {
                                temp["max"].asDouble(), temp["min"].asDouble(),
                                0.0 } });
    }

    return result;
}

http::Request::Progress::Next Client::progress_report(
        const http::Request::Progress&) {

    return cancelled_ ?
            http::Request::Progress::Next::abort_operation :
            http::Request::Progress::Next::continue_operation;
}

void Client::cancel() {
    cancelled_ = true;
}

Config::Ptr Client::config() {
    return config_;
}
