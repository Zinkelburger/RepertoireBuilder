CC = g++
CFLAGS = --std=c++17 -Wall -Werror -pedantic -O3
LIB = -lcurl

.PHONY: all clean lint

all: repertoireBuilder lint

%.o: %.cpp
	$(CC) $(CFLAGS) -c $<

repertoireBuilder: main.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIB)

lint:
	cpplint *.cpp *.hpp

clean:
	rm *.o repertoireBuilder   