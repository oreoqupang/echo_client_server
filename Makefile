TARGET = echo_server echo_client

all : $(TARGET)

echo_server:
	g++ -o $@ $@.cpp -lpthread

echo_client:
	g++ -o $@ $@.cpp -lpthread

clean:
	rm -f echo_server
	rm -f echo_client
