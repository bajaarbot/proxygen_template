#include "common.h"
#include "server.h"

#include <exception>

static constexpr char JSON_content_type[] = "application/json; charset=utf-8";
static constexpr char MAXAGE_4hour[] = "max-age=14400, public";
static constexpr char MAXAGE_1day[] = "max-age=86400, public";

namespace Handler {

void HelloV1::onRequest(std::unique_ptr<HTTPMessage> headers) noexcept {
    const char* const route_string = headers->getURL().c_str();
    strncpy(parameter, route_string + routelen, MAXLEN - 1);
    //auto request = headers->getMethod();
    //if (request == HTTPMethod::GET) {
    //         ....
    //}
}

void HelloV1::onEOM() noexcept {
    folly::fbstring out{};

    auto task = executor()->addFuture([&out, input = parameter]() {
        // Sanitize input!
        
        // Process
        
        // Prepare output
        out = folly::sformat("{{\"response\":\"HelloV1 {}\"}}", input);
    }).onError([&out](const std::exception &e) {
        // DB error, or some such failure
        out = "{\"response\": \"server failed\"}";
        LOG(ERROR) << e.what() << '\n';
    });

    task.wait();
    
    ResponseBuilder(downstream_)
        .status(200, "OK")
        .header(HTTPHeaderCode::HTTP_HEADER_CONTENT_TYPE, JSON_content_type)
        .header(HTTPHeaderCode::HTTP_HEADER_CACHE_CONTROL, MAXAGE_4hour)
        .body(std::move(out))
        .sendWithEOM();
}

void DefaultHello::onRequest(std::unique_ptr<HTTPMessage> headers) noexcept {
    const char* const route_string = headers->getURL().c_str();
    strncpy(parameter, route_string + routelen, MAXLEN - 1);
}

void DefaultHello::onEOM() noexcept {
    folly::fbstring out{};

    auto task = executor()->addFuture([&out, input = parameter]() {
        // Sanitize input!
        
        
        // Prepare output
        out = folly::sformat("{{\"response\":\"DefaultHello {}\"}}", input);
    }).onError([&out](const std::exception &e) {
        // DB error, or some such failure
        out = "{\"response\": \"server failed\"}";
        LOG(ERROR) << e.what() << '\n';
    });

    task.wait();
    
    ResponseBuilder(downstream_)
        .status(200, "OK")
        .header(HTTPHeaderCode::HTTP_HEADER_CONTENT_TYPE, JSON_content_type)
        .header(HTTPHeaderCode::HTTP_HEADER_CACHE_CONTROL, MAXAGE_4hour)
        .body(std::move(out))
        .sendWithEOM();
}

void Hola::onRequest(std::unique_ptr<HTTPMessage>) noexcept {
    // Empty
}


void Hola::onEOM() noexcept {
    
    ResponseBuilder(downstream_)
        .status(200, "OK")
        .header(HTTPHeaderCode::HTTP_HEADER_CONTENT_TYPE, JSON_content_type)
        .header(HTTPHeaderCode::HTTP_HEADER_CACHE_CONTROL, MAXAGE_4hour)
        .body("{\"response\": \"No habla espanol\"}")
        .sendWithEOM();
}


}
