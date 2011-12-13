PROG=	mongodb-rest
CFLAGS=	-W -Wall -Imongoose -pthread -g
CXXFLAGS=$(CFLAGS)
LIBS=-ldl

all: $(PROG)

mongoose.o: mongoose.c
	$(CC) $(CFLAGS) -o $@ -c $^

main.o: main.cpp

$(PROG): main.o mongoose.o
	g++ $(CXXFLAGS) -o $@ $^ $(LIBS)
