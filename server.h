#pragma once
using namespace proxygen;


namespace Handler {
class App;
}

/**
 * Request routing
 * Stores routes, and owns database and threadpools
 * */
class ServiceFactory final : public proxygen::RequestHandlerFactory {
    friend class Handler::App;
public:
    explicit ServiceFactory();

    void onServerStart(folly::EventBase*) noexcept override { }

    void onServerStop() noexcept override { }

    /* Route incoming to appropriate RequestHandler */
    RequestHandler* onRequest(RequestHandler*, HTTPMessage* headers) noexcept override;

protected:
    // Database connections, or any other shared resources
    const std::unique_ptr<wangle::FutureExecutor<wangle::CPUThreadPoolExecutor>> threadPool_ptr;
};


/* Base class for all services */
namespace Handler {
class App: public proxygen::RequestHandler {
public:
    explicit App(ServiceFactory *parent) : _parent(parent) {}
    void onBody(std::unique_ptr<folly::IOBuf>) noexcept override {/* Ignore body */}
    void requestComplete() noexcept override {delete this;}
    void onError(proxygen::ProxygenError) noexcept override{delete this;}
    void onUpgrade(UpgradeProtocol) noexcept override {/* Ignore upgrade */}
    void onRequest(std::unique_ptr<HTTPMessage>) noexcept override {}
    void onEOM() noexcept override {}

    static constexpr const char prefix[] = "/";

protected:
    // Getters for resources from ServiceFactory
    inline wangle::FutureExecutor<wangle::CPUThreadPoolExecutor>* executor() const noexcept {
        return _parent->threadPool_ptr.get();
    }
private:
    ServiceFactory *const _parent{nullptr};
};

/* ------------------------ List of services ----------------- */

class HelloV1 final : public App {
public:
    explicit HelloV1(ServiceFactory *parent) : App(parent)  {}
    void onRequest(std::unique_ptr<HTTPMessage> headers) noexcept override;
    void onEOM() noexcept override;
    static constexpr const char prefix[] = "/hello/v1/"; // String parameter
    static constexpr unsigned int routelen = sizeof(prefix) - 1;
private:
    const static unsigned int MAXLEN = 256;
    char parameter[MAXLEN] = {};
};

class DefaultHello final : public App {
public:
    explicit DefaultHello(ServiceFactory *parent) : App(parent)  {}
    void onRequest(std::unique_ptr<HTTPMessage> headers) noexcept override;
    void onEOM() noexcept override;
    static constexpr const char prefix[] = "/hello/"; // String parameter
    static constexpr unsigned int routelen = sizeof(prefix) - 1;
private:
    const static unsigned int MAXLEN = 256;
    char parameter[MAXLEN] = {};
};

class Hola final : public App {
public:
    explicit Hola(ServiceFactory *parent) : App(parent)  {}
    void onRequest(std::unique_ptr<HTTPMessage> headers) noexcept override;
    void onEOM() noexcept override;
    static constexpr const char prefix[] = "/hola"; // No parameters
};


}
