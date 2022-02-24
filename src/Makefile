CPPFLAGS=-std=gnu++11 -pthread -ggdb3
myproxy: proxy.cpp cache.hpp fifo.hpp logger.hpp myexception.hpp request.hpp response.hpp routine.hpp socket.hpp timeTool.hpp
	g++ $(CPPFLAGS) -o myproxy proxy.cpp

.PHONY: clean
clean:
	rm -f *.o *~ myproxy
