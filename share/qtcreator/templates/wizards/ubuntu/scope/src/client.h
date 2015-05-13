#ifndef CLIENT_H_
#define CLIENT_H_

#include <atomic>
#include <deque>
#include <map>
#include <string>
@if "%ContentType%".substring(0, "network".length) === "network"
#include <core/net/http/request.h>
#include <core/net/uri.h>

@if "%ContentType%" == "network-netcpp-json"
namespace Json {
class Value;
}
@elsif "%ContentType%" == "network-netcpp-qjson"
#include <QJsonDocument>
@elsif "%ContentType%" == "network-netcpp-qxml"
#include <QXmlStreamReader>
@endif
@endif

/**
 * Provide a nice way to access the HTTP API.
 *
 * We don't want our scope's code to be mixed together with HTTP and JSON handling.
 */
class Client {
public:

    /**
     * Client configuration
     */
    struct Config {
        typedef std::shared_ptr<Config> Ptr;

        // The root of all API request URLs
        std::string apiroot { "http://api.openweathermap.org" };

        // The custom HTTP user agent string for this library
        std::string user_agent { "example-network-scope 0.1; (foo)" };
    };

@if "%ContentType%" == "empty"
    /**
     * Result struct
     */
    struct Result {
        std::string uri;
        std::string title;
        std::string art;
        std::string subtitle;
        std::string description;
    };

    /**
     * A list of weather information
     */
    typedef std::deque<Result> ResultList;
@endif
@if "%ContentType%".substring(0, "network".length) === "network"
    /**
     * Information about a City
     */
    struct City {
        unsigned int id;
        std::string name;
        std::string country;
    };

    /**
     * Temperature information for a day.
     */
    struct Temp {
        double max;
        double min;
        double cur;
    };

    /**
     * Weather information for a day.
     */
    struct Weather {
        unsigned int id;
        std::string main;
        std::string description;
        std::string icon;
        Temp temp;
    };

    /**
     * A list of weather information
     */
    typedef std::deque<Weather> WeatherList;

    /**
     * Weather information about the current day
     */
    struct Current {
        City city;
        Weather weather;
    };

    /**
     * Forecast information about a city
     */
    struct Forecast {
        City city;
        WeatherList weather;
    };
@endif

    Client(Config::Ptr config);

    virtual ~Client() = default;

@if "%ContentType%" == "empty"
    /**
     * Search for results
     */
    virtual ResultList search(const std::string &query);
@endif
@if "%ContentType%".substring(0, "network".length) === "network"
    /**
     * Get the current weather for the specified location
     */
    virtual Current weather(const std::string &query);

    /**
     * Get the weather forecast for the specified location and duration
     */
    virtual Forecast forecast_daily(const std::string &query, unsigned int days = 7);
@endif

    /**
     * Cancel any pending queries (this method can be called from a different thread)
     */
    virtual void cancel();

    virtual Config::Ptr config();

protected:
@if "%ContentType%" == "network-netcpp-json"
    void get(const core::net::Uri::Path &path,
            const core::net::Uri::QueryParameters &parameters,
            Json::Value &root);
@elsif "%ContentType%" == "network-netcpp-qjson"
    void get(const core::net::Uri::Path &path,
                const core::net::Uri::QueryParameters &parameters,
                QJsonDocument &root);
@elsif "%ContentType%" == "network-netcpp-qxml"
    void get(const core::net::Uri::Path &path,
                const core::net::Uri::QueryParameters &parameters,
                QXmlStreamReader &reader);
@endif
@if "%ContentType%".substring(0, "network".length) === "network"
    /**
     * Progress callback that allows the query to cancel pending HTTP requests.
     */
    core::net::http::Request::Progress::Next progress_report(
            const core::net::http::Request::Progress& progress);
@endif

    /**
     * Hang onto the configuration information
     */
    Config::Ptr config_;

    /**
     * Thread-safe cancelled flag
     */
    std::atomic<bool> cancelled_;
};

#endif // CLIENT_H_
