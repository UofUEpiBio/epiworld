all: main.o

main.o: main.cpp
	g++ -std=c++14 -Wall -O2 -g -pedantic main.cpp -o main.o

main.a: main.cpp
	clang++ -Depiworld_fast_int=int -std=c++14 -O2 main.cpp -o main.a

.PHONY: all
