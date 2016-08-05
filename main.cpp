#include "common.h"
#include "server.h"
#include "boost/static_assert.hpp"


using namespace proxygen;

using folly::EventBase;
using folly::EventBaseManager;
using folly::SocketAddress;

using Protocol = HTTPServer::Protocol;

DEFINE_int32(http_port, 11000, "Port to listen on with HTTP protocol");
DEFINE_string(ip, "127.0.0.1", "IP/Hostname to bind to");
DEFINE_int32(threads, 0, "Number of threads to listen on. Numbers <= 0 "
             "will use the number of cores on this machine.");

const static size_t NUM_THREADS_IN_POOL = 20;

// All static variables here - add one for every route in services.cpp
namespace Handler {
    constexpr const char HelloV1::prefix[];
    constexpr const char DefaultHello::prefix[];
    constexpr const char Hola::prefix[];
}

class UnknownURL : public proxygen::RequestHandler {
public:
    explicit UnknownURL() {}
    void onRequest(std::unique_ptr<HTTPMessage>) noexcept override {}
    void onBody(std::unique_ptr<folly::IOBuf>) noexcept override {/* Ignore body */}
    void onEOM() noexcept override {
        ResponseBuilder(downstream_)
            .status(404, "Not Found")
            .body("")
            .sendWithEOM();
    }
    void requestComplete() noexcept override {delete this;}
    void onError(proxygen::ProxygenError) noexcept override{delete this;}
    void onUpgrade(UpgradeProtocol) noexcept override {/* Ignore upgrade */}
};

class ServiceFactory;


template<typename T>
static Handler::App* handler_(ServiceFactory *parent) {
    //LOG(INFO) << "Creating handler for " << typeid(T).name() << '\n';
    return new T(parent);
}


class Prefix { 
private:
    const char* const p_;
    const std::size_t sz_;
public:
    Handler::App* (*handler_) (ServiceFactory *);

    template<std::size_t N>
    constexpr Prefix(const char(&a)[N], Handler::App* (*fn)(ServiceFactory *)) : // ctor
    p_(a), sz_(N-1), handler_(fn) {}

    constexpr Prefix(const Prefix& ppair) : p_(ppair.p_), sz_(ppair.sz_), handler_(ppair.handler_) {}

    constexpr const char* prefix() const noexcept {
        return p_;
    }

    inline constexpr bool matches(const char *input) const noexcept {
        return (strncmp(p_, input, sz_) == 0);
    }

    constexpr std::size_t len() const noexcept { return sz_; } // len()
};


template<std::size_t N>
class PrefixRouter {
private:
    const Prefix routes[N];
public:
    using length_type = decltype(N);
    constexpr PrefixRouter(const Prefix (&_routes)[N]) : routes(_routes)  { }

    constexpr const Prefix* operator[](std::size_t n) const {
        return n < N ? &routes[n] :
        throw std::out_of_range("");
    }

    inline constexpr RequestHandler* get_handler(const char* str, ServiceFactory *parent) const noexcept {
        for (length_type i = 0; i < N; ++i) {
            if (routes[i].matches(str)) {
                return routes[i].handler_(parent);
            }
        }
        return new UnknownURL();
    }

    constexpr bool verify_routes() const {
        for (length_type i = 0; i < N; i++) {
            const char* const prefix = routes[i].prefix();
            for (std::size_t j = 0; j <= i; ++j) {
                // If you get an error here, you forgot to define
                // ::prefix static for a new route at the top of this file
                auto matching = routes[j].matches(prefix);
                if (matching) {
                    if (i != j)
                        throw std::invalid_argument("route order incorrect");
                    continue;
                }
            }
        }
        return true;
    }

    constexpr length_type len() const noexcept {
        return N;
    }

};


ServiceFactory::ServiceFactory() : threadPool_ptr(std::make_unique<wangle::FutureExecutor<wangle::CPUThreadPoolExecutor>>(NUM_THREADS_IN_POOL))  {
    // Setup any other shared resource here. Eg - database
}


#define ROUTE(x) Prefix(x::prefix, handler_<x>)
RequestHandler* ServiceFactory::onRequest(RequestHandler*, HTTPMessage* headers) noexcept {
      // Find first matching prefix, so order is important
      constexpr PrefixRouter<3> router ({
              ROUTE(Handler::HelloV1),
              ROUTE(Handler::DefaultHello),
              ROUTE(Handler::Hola),
     });
     BOOST_STATIC_ASSERT(router.verify_routes());


     return router.get_handler(headers->getURL().c_str(), this);
}


int main(int argc, char* argv[]) {
#ifdef PRODUCTION
  FLAGS_log_dir = "/srv/myapp/logs";
#else
  FLAGS_logtostderr = 1;
#endif
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();

  std::vector<HTTPServer::IPConfig> IPs = {
    {SocketAddress(FLAGS_ip, FLAGS_http_port, true), Protocol::HTTP},
  };

  if (FLAGS_threads <= 0) {
    FLAGS_threads = sysconf(_SC_NPROCESSORS_ONLN);
    CHECK(FLAGS_threads > 0);
  }


  HTTPServerOptions options;
  options.threads = static_cast<size_t>(FLAGS_threads);
  options.idleTimeout = std::chrono::milliseconds(60000);
  options.shutdownOn = {SIGINT, SIGTERM};
  options.enableContentCompression = false;
  options.handlerFactories = RequestHandlerChain()
      .addThen<ServiceFactory>()
      .build();

  HTTPServer server(std::move(options));
  server.bind(IPs);

  // Start HTTPServer mainloop in a separate thread
  std::thread t([&] () {
    server.start();
  });

  t.join();
  return 0;
}
