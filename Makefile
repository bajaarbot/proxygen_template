# Update DEPLOY_CPU to match your target. Eg- ivybridge, haswell, broadwell, skylake, ...
DEPLOY_CPU = sandybridge
LIBDIR = /usr/local/lib
INCDIR = /usr/local/include

SAFETY_FLAGS= -DFORTIFY_SOURCE=2 -fstack-protector-strong -Wl,-z,nodlopen,-z,nodump,-z,noexecstack,-z,relro,-z,now
WARN_FLAGS = -std=gnu++14 -Winvalid-pch -Wall -Wextra -Wconversion -Wformat=2 -fno-common -Wstrict-overflow -Wtrampolines -Wsign-promo
INCLUDES = -I$(INCDIR) -I.
SRC = main.cpp services.cpp

# For dev
OPT = -O2 -march=native -mtune=native
LIBS = -L $(LIBDIR) -lproxygenhttpserver -lwangle -lfolly -lglog -lgflags -pthread

# For deployment, create a binary with all non-standard libraries included. You might need to fix the paths below.
DEPLOY_OPT = -O2 -march=$(DEPLOY_CPU) -mtune=$(DEPLOY_CPU)
DEPLOY_LIBS = -static-libgcc -static-libstdc++ $(LIBDIR)/libproxygenhttpserver.a $(LIBDIR)/libproxygenlib.a $(LIBDIR)/libwangle.a $(LIBDIR)/libfolly.a $(LIBDIR)/libglog.a $(LIBDIR)/libgflags.a /usr/lib/libboost_system.a /usr/lib/libboost_context.a  $(LIBDIR)/libevent.a $(LIBDIR)/libdouble-conversion.a $(LIBDIR)/libjemalloc.a /usr/lib/libz.a /usr/lib/libiberty.a -lunwind -llzma -lssl -lcrypto -pthread -ldl


default: Hello

common.h.gch: common.h
	@echo ================= Compiling common header  ======================
	@g++ $(INCLUDES) $(WARN_FLAGS) $(OPT) $(SAFETY_FLAGS) -D_REENTRANT -o $@  -c $< 


Hello: $(SRC) common.h.gch server.h
	@echo ================= Compiling $@ ======================
	@g++ $(INCLUDES) $(WARN_FLAGS) $(OPT) $(SAFETY_FLAGS) $(SRC) -o $@  $(LIBS)

Hello_partstatic: $(SRC) server.h
	@echo ================= Compiling $@ ======================
	@g++ $(INCLUDES) -DPRODUCTION $(WARN_FLAGS) $(SAFETY_FLAGS) $(DEPLOY_OPT) $(SRC) -o $@  $(DEPLOY_LIBS)
	@strip -s $@


clean:
	@rm -f *.o *.gch Hello Hello_partstatic
