proxy: HttpResp.cpp HttpRequest.cpp parser.cpp proxy.cpp cache.cpp main.cpp
	g++ -std=gnu++11 -ggdb3 -o proxy HttpResp.cpp HttpRequest.cpp parser.cpp proxy.cpp cache.cpp main.cpp -pthread
server: server.cpp
	g++ -std=gnu++11 -ggdb3 -o server server.cpp
clean:
	rm -f *.cpp~ *.h~ *~
