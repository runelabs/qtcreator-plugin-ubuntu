#ifndef API_CLIENT_H_
#define API_CLIENT_H_

#include <api/config.h>

#include <atomic>
#include <deque>
#include <map>
#include <string>
#include <core/net/http/request.h>
#include <core/net/uri.h>

namespace Json {
class Value;
}

namespace api {

class Client {
public:
    struct City {
        unsigned int id;
        std::string name;
        std::string country;
    };

    struct Temp {
        double max;
        double min;
        double cur;
    };

    struct Weather {
        unsigned int id;
        std::string main;
        std::string description;
        std::string icon;
        Temp temp;
    };

    typedef std::deque<Weather> WeatherList;

    struct Current {
        City city;
        Weather weather;
    };

    struct Forecast {
        City city;
        WeatherList weather;
    };

    Client(Config::Ptr config);

    virtual ~Client() = default;

    virtual Current weather(const std::string &query);

    virtual Forecast forecast_daily(const std::string &query, unsigned int days = 7);

    virtual void cancel();

    virtual Config::Ptr config();

protected:
    void get(const core::net::Uri::Path &path,
            const core::net::Uri::QueryParameters &parameters,
            Json::Value &root);

    core::net::http::Request::Progress::Next progress_report(
            const core::net::http::Request::Progress& progress);

    Config::Ptr config_;

    std::atomic<bool> cancelled_;
};

}

#endif // API_CLIENT_H_
