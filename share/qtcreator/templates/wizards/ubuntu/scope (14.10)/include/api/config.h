#ifndef API_CONFIG_H_
#define API_CONFIG_H_

#include <memory>
#include <string>

namespace api {

struct Config {
    typedef std::shared_ptr<Config> Ptr;

@if "%ContentType%".substring(0, "network".length) === "network"
    /*
     * The root of all API request URLs
     */
    std::string apiroot { "http://api.openweathermap.org" };

    /*
     * The custom HTTP user agent string for this library
     */
    std::string user_agent { "example-network-scope 0.1; (foo)" };
@endif
};

}

#endif /* API_CONFIG_H_ */
