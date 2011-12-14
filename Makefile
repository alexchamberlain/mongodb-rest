PROG=	./mongodb-rest

CC=gcc
CXX=g++
CFLAGS=	-W -Wall -pthread -g
CXXFLAGS=$(CFLAGS)
LIBS=-ldl -lmongoclient -lboost_regex -lboost_thread\
     -lboost_program_options -lboost_filesystem

all: $(PROG)

mongoose.o: mongoose.c
	$(CC) $(CFLAGS) -o $@ -c $^

main.o: main.cpp

$(PROG): main.o mongoose.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

clean:
	rm *.o $(PROG)

run:
	$(PROG)

.PHONY: clean
