all: startmirror createlog

%: %.cpp
	g++ $< -o $@ -std=c++14 -I/usr/include/jsoncpp -I$(HOME)/local/include -L$(HOME)/local/lib -lcpp_redis -ltacopie -lpthread -lrestclient-cpp -ljsoncpp

clean:
	rm -f startmirror createlog
