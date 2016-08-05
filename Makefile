CFLAGS = $(OPT) -std=gnu++14 -Winvalid-pch -Wall
INCLUDES = -I/usr/local/include -I.
LIBDIR = /usr/local/lib
LIBS = -L $(LIBDIR) -lproxygenhttpserver -lwangle -lfolly -lglog -lgflags -pthread
SRC = main.cpp services.cpp




default: Hello

common.h.gch: common.h
	@echo ================= Compiling common header  ======================
	@g++ $(INCLUDES) $(CFLAGS) -D_REENTRANT -o $@  -c $< 


Hello: $(SRC) common.h.gch server.h
	@echo ================= Compiling $@ ======================
	g++ $(INCLUDES) $(CFLAGS) $(SRC) -o $@  $(LIBS)

clean:
	@rm -f *.o *.gch Hello
