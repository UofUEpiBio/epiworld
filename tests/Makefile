all: main.o

main.o: main.cpp
	g++ -std=c++14 -Wall -Depiworld_double=double -O2 -g -pedantic main.cpp -o main.o

main.a: main.cpp
	clang++ -std=c++14 -Depiworld_double=double -O2 main.cpp -o main.a

.PHONY: all
