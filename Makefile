PROG=	./mongodb-rest

CC=gcc
CXX=g++
CFLAGS=	-W -Wall -pthread -g
CXXFLAGS=$(CFLAGS)
LIBS=-ldl -lmongoclient -lboost_regex -lboost_thread\
     -lboost_program_options -lboost_filesystem -lz

all: $(PROG)

mongoose.o: mongoose.c
	$(CC) $(CFLAGS) -o $@ -c $^

main.o: main.cpp

request_handler.o: request_handler.cpp

response.o: response.cpp

$(PROG): main.o request_handler.o mongoose.o response.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

clean:
	rm *.o $(PROG)

run:
	$(PROG)

.PHONY: clean
