#include <api/client.h>

@if "%ContentType%".substring(0, "network-netcpp".length) === "network-netcpp"
#include <core/net/error.h>
#include <core/net/http/client.h>
#include <core/net/http/content_type.h>
#include <core/net/http/response.h>
@if "%ContentType%" == "network-netcpp-json"
#include <json/json.h>
@elsif "%ContentType%" == "network-netcpp-qjson"
#include <QVariantMap>
@endif

namespace http = core::net::http;
namespace net = core::net;
@if "%ContentType%" == "network-netcpp-json"
namespace json = Json;
@endif
@endif

using namespace api;
using namespace std;

Client::Client(Config::Ptr config) :
        config_(config), cancelled_(false) {
}

@if "%ContentType%" == "network-netcpp-qxml"
namespace {
static QString readText(QXmlStreamReader& xml)
{
    xml.readNext();

    if (xml.tokenType() != QXmlStreamReader::Characters) {
        return QString();
    }

    return xml.text().toString();
}

static void parseCity(Client::City& city, QXmlStreamReader& xml)
{
    QXmlStreamAttributes attributes = xml.attributes();
    if (attributes.hasAttribute("id")) {
        city.id = attributes.value("id").toUInt();
    }
    if (attributes.hasAttribute("name")) {
        city.name = attributes.value("name").toString().toStdString();
    }

    xml.readNext();

    while (!xml.atEnd() && !(xml.isEndElement() && xml.name() == "city")) {
        if (xml.isStartElement()) {
            if (xml.name() == "country") {
                city.country = readText(xml).toStdString();
            }
        }
        xml.readNext();
    }
}

static void parseWeather(Client::Weather& weather, QXmlStreamReader& xml)
{
    QXmlStreamAttributes attributes = xml.attributes();
    if (attributes.hasAttribute("id")) {
        weather.id = attributes.value("id").toUInt();
    }
    if (attributes.hasAttribute("value")) {
        weather.description = attributes.value("value").toString().toStdString();
    }
    if (attributes.hasAttribute("icon")) {
        weather.icon = "http://openweathermap.org/img/w/"
                    + attributes.value("icon").toString().toStdString() + ".png";
    }

    xml.readNext();

    while (!xml.atEnd() && !(xml.isEndElement() && xml.name() == "weather")) {
        xml.readNext();
    }
}

static void parseTemperature(Client::Temp& temp, QXmlStreamReader& xml)
{
    QXmlStreamAttributes attributes = xml.attributes();
    if (attributes.hasAttribute("value")) {
        temp.cur = attributes.value("value").toDouble();
    }
    if (attributes.hasAttribute("max")) {
        temp.max = attributes.value("max").toDouble();
    }
    if (attributes.hasAttribute("min")) {
        temp.min = attributes.value("min").toDouble();
    }

    xml.readNext();

    while (!xml.atEnd() && !(xml.isEndElement() && xml.name() == "temperature")) {
        xml.readNext();
    }
}

static void parseLocation(Client::City& city, QXmlStreamReader& xml)
{
    xml.readNext();

    while (!xml.atEnd() && !(xml.isEndElement() && xml.name() == "location")) {
        if (xml.name() == "name") {
            city.name = readText(xml).toStdString();
        } else if (xml.name() == "country") {
            city.country = readText(xml).toStdString();
        }

        xml.readNext();
    }
}

static void parseTime(Client::Weather& weather, QXmlStreamReader& xml)
{
    xml.readNext();

    while (!xml.atEnd() && !(xml.isEndElement() && xml.name() == "time"))
    {
        if (xml.name() == "symbol") {
            QXmlStreamAttributes attributes = xml.attributes();
            if (attributes.hasAttribute("name")) {
                weather.description = attributes.value("name").toString().toStdString();
            }
            if (attributes.hasAttribute("var")) {
                weather.icon = "http://openweathermap.org/img/w/"
                        + attributes.value("var").toString().toStdString() + ".png";
            }
        } else if (xml.name() == "temperature") {
            QXmlStreamAttributes attributes = xml.attributes();
            if (attributes.hasAttribute("max")) {
                weather.temp.max = attributes.value("max").toDouble();
            }
            if (attributes.hasAttribute("min")) {
                weather.temp.min = attributes.value("min").toDouble();
            }
            if (attributes.hasAttribute("day")) {
                weather.temp.cur = attributes.value("day").toDouble();
            }
        }

        xml.readNext();
    }
}

static void parseForecast(Client::WeatherList& weather_list, QXmlStreamReader& xml)
{
    xml.readNext();

    unsigned int weather_id = 1000000;

    while (!xml.atEnd() && !(xml.isEndElement() && xml.name() == "forecast"))
    {
        if (xml.name() == "time") {
            Client::Weather weather;
            weather.id = weather_id++;
            parseTime(weather, xml);
            weather_list.emplace_back(weather);
        }
        xml.readNext();
    }
}

}
@endif

@if "%ContentType%" == "empty"
Client::ResultList Client::search(const string &query) {
    ResultList results;

    // This is the method that we will call from the Query class.
    // It just returns some results.
    // You can add here your code to get results from an http API, from your local disk
    // or anywhere.

    // In this case we just create some results withouth accessing any other source of
    // data
    {
        Result result;
        result.uri = "uri";
        result.title = query;
        result.art = "art.png";
        result.subtitle = "subtitle";
        result.description = "description";
        results.emplace_back(result);
    }
    {
        Result result;
        result.uri = "uri2";
        result.title = query;
        result.art = "art2.png";
        result.subtitle = "subtitle2";
        result.description = "description2";
        results.emplace_back(result);
    }

    return results;
}
@endif
@if "%ContentType%".substring(0, "network-netcpp".length) === "network-netcpp"
@if "%ContentType%" == "network-netcpp-json"
void Client::get(const net::Uri::Path &path,
        const net::Uri::QueryParameters &parameters, json::Value &root) {
@elsif "%ContentType%" == "network-netcpp-qjson"
void Client::get(const net::Uri::Path &path,
        const net::Uri::QueryParameters &parameters, QJsonDocument &root) {
@elsif "%ContentType%" == "network-netcpp-qxml"
void Client::get(const net::Uri::Path &path,
        const net::Uri::QueryParameters &parameters, QXmlStreamReader &reader) {
@endif
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
            throw domain_error(response.body);
        }
@if "%ContentType%" == "network-netcpp-json"
        // Parse the JSON from the response
        json::Reader reader;
        reader.parse(response.body, root);

        // Open weather map API error code can either be a string or int
        json::Value cod = root["cod"];
        if ((cod.isString() && cod.asString() != "200")
                || (cod.isUInt() && cod.asUInt() != 200)) {
            throw domain_error(root["message"].asString());
        }
@elsif "%ContentType%" == "network-netcpp-qjson"
        // Parse the JSON from the response
        root = QJsonDocument::fromJson(response.body.c_str());

        // Open weather map API error code can either be a string or int
        QVariant cod = root.toVariant().toMap()["cod"];
        if ((cod.canConvert<QString>() && cod.toString() != "200")
                || (cod.canConvert<unsigned int>() && cod.toUInt() != 200)) {
            throw domain_error(root.toVariant().toMap()["message"].toString().toStdString());
        }
@elsif "%ContentType%" == "network-netcpp-qxml"
        // Parse the Xml from the response
        reader.addData(response.body.c_str());
@endif
    } catch (net::Error &) {
    }
}

Client::Current Client::weather(const string& query) {
    // This is the method that we will call from the Query class.
    // It connects to an HTTP source and returns the results.

@if "%ContentType%" == "network-netcpp-json"

    // In this case we are going to retrieve JSON data.
    json::Value root;
@elsif "%ContentType%" == "network-netcpp-qjson"

    // In this case we are going to retrieve JSON data.
    QJsonDocument root;
@elsif "%ContentType%" == "network-netcpp-qxml"

    // In this case we are going to retrieve XML data.
    QXmlStreamReader root;
@endif

    // Build a URI and get the contents.
    // The fist parameter forms the path part of the URI.
    // The second parameter forms the CGI parameters.
    get( { "data", "2.5", "weather" },
         { { "q", query }, { "units", "metric" }
         , { "APPID", "2b12bf09b4e0ab0c1aa5e32a9a3f0cdc" }
@if "%ContentType%" == "network-netcpp-qxml"
         , { "mode", "xml" }
@endif
         }, root);
    // e.g. http://api.openweathermap.org/data/2.5/weather?q=QUERY&units=metric

    Current result;

@if "%ContentType%" == "network-netcpp-json"
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
@elsif "%ContentType%" == "network-netcpp-qjson"
    // Read out the city we found
    QVariantMap variant = root.toVariant().toMap();
    QVariantMap sys = variant["sys"].toMap();
    result.city.id = sys["id"].toUInt();
    result.city.name = variant["name"].toString().toStdString();
    result.city.country = sys["country"].toString().toStdString();

    // Read the weather
    QVariantMap weather = variant["weather"].toList().first().toMap();
    result.weather.id = weather["id"].toUInt();
    result.weather.main = weather["main"].toString().toStdString();
    result.weather.description = weather["description"].toString().toStdString();
    result.weather.icon = "http://openweathermap.org/img/w/"
            + weather["icon"].toString().toStdString() + ".png";

    // Read the temps
    QVariantMap main = variant["main"].toMap();
    result.weather.temp.cur = main["temp"].toDouble();
    result.weather.temp.max = main["temp_max"].toDouble();
    result.weather.temp.min = main["temp_min"].toDouble();
@elsif "%ContentType%" == "network-netcpp-qxml"
    while (!root.atEnd() && !root.hasError()) {
        QXmlStreamReader::TokenType token = root.readNext();

        /* If token is just StartDocument, we'll go to next.*/
        if (token == QXmlStreamReader::StartDocument) {
            continue;
        }

        /* If token is StartElement, we'll see if we can read it.*/
        if (token == QXmlStreamReader::StartElement) {
            if (root.name() == "current") {
                root.readNext();
            } else if (root.name() == "city") {
                parseCity(result.city, root);
            } else if (root.name() == "weather") {
                parseWeather(result.weather, root);
            } else if (root.name() == "temperature") {
                parseTemperature(result.weather.temp, root);
            }
        }
    }

    if (root.hasError()) {
        throw domain_error(root.errorString().toStdString());
    }
@endif
    return result;
}

Client::Forecast Client::forecast_daily(const string& query, unsigned int cnt) {
@if "%ContentType%" == "network-netcpp-json"
    json::Value root;
@elsif "%ContentType%" == "network-netcpp-qjson"
    QJsonDocument root;
@elsif "%ContentType%" == "network-netcpp-qxml"
    QXmlStreamReader root;
@endif

    // Build a URI and get the contents
    // The fist parameter forms the path part of the URI.
    // The second parameter forms the CGI parameters.
    get( { "data", "2.5", "forecast", "daily" },
         { { "q", query }, { "units", "metric" }, { "cnt", to_string(cnt) }
         , { "APPID", "2b12bf09b4e0ab0c1aa5e32a9a3f0cdc" }
@if "%ContentType%" == "network-netcpp-qxml"
         , { "mode", "xml" }
@endif
         }, root);
    // e.g. http://api.openweathermap.org/data/2.5/forecast/daily/?q=QUERY&units=metric&cnt=7

    Forecast result;

@if "%ContentType%" == "network-netcpp-json"
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
@elsif "%ContentType%" == "network-netcpp-qjson"
    QVariantMap variant = root.toVariant().toMap();

    // Read out the city we found
    QVariantMap city = variant["city"].toMap();
    result.city.id = city["id"].toUInt();
    result.city.name = city["name"].toString().toStdString();
    result.city.country = city["country"].toString().toStdString();

    // Iterate through the weather data
    for (const QVariant &i : variant["list"].toList()) {
        QVariantMap item = i.toMap();

        // Extract the first weather item
        QVariantList weather_list = item["weather"].toList();
        QVariantMap weather = weather_list.first().toMap();

        // Extract the temperature data
        QVariantMap temp = item["temp"].toMap();

        // Add a result to the weather list
        result.weather.emplace_back(
                Weather { weather["id"].toUInt(), weather["main"].toString().toStdString(),
                        weather["description"].toString().toStdString(),
                        "http://openweathermap.org/img/w/"
                                + weather["icon"].toString().toStdString() + ".png", Temp {
                                temp["max"].toDouble(), temp["min"].toDouble(),
                                0.0 } });
    }
@elsif "%ContentType%" == "network-netcpp-qxml"
    while (!root.atEnd() && !root.hasError()) {
        QXmlStreamReader::TokenType token = root.readNext();

        /* If token is just StartDocument, we'll go to next.*/
        if (token == QXmlStreamReader::StartDocument) {
            continue;
        }

        /* If token is StartElement, we'll see if we can read it.*/
        if (token == QXmlStreamReader::StartElement) {
            if (root.name() == "weatherdata") {
                root.readNext();
            } else if (root.name() == "location") {
                parseLocation(result.city, root);
            } else if (root.name() == "forecast") {
                parseForecast(result.weather, root);
            }
        }
    }

    if (root.hasError()) {
        throw domain_error(root.errorString().toStdString());
    }
@endif

    return result;
}

http::Request::Progress::Next Client::progress_report(
        const http::Request::Progress&) {

    return cancelled_ ?
            http::Request::Progress::Next::abort_operation :
            http::Request::Progress::Next::continue_operation;
}
@endif

void Client::cancel() {
    cancelled_ = true;
}

Config::Ptr Client::config() {
    return config_;
}
