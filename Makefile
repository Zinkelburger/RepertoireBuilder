CC = g++
CFLAGS = --std=c++17 -Wall -Werror -pedantic -g
LIB = -lpqxx -lpq

.PHONY: all clean lint

all: repertoireBuilder parser lint

%.o: %.cpp
	$(CC) $(CFLAGS) -c $<

# thc.o has warnings, can't use -Werror
thc.o: thc.cpp
	$(CC) --std=c++17 -pedantic -O3 -c $< 2> /dev/null

repertoireBuilder: main.o thc.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIB)

parser: parse.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIB)

lint:
	cpplint *.cpp *.hpp

clean:
	rm *.o repertoireBuilder parser