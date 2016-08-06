###Template code for HTTP server based on Proxygen

#### Prefix Routing

     
	
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
	

 The `PrefixRouter` is initialized, built and verified at compile time. Each Handler derives from `App` and contains the route-prefix as a static member:

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
	  }


#### ThreadPools

The ServiceFactory creates a thread-pool and passes it on to individual route handlers for async processing.

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
	
