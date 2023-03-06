all: main.o

main.o: main.cpp
	g++ -std=c++14 -Wall -Wextra -O2 -g -pedantic main.cpp -o main.o

main.a: main.cpp
	clang++ -std=c++14 -Wall -Wextra -Wpedantic -O2 main.cpp -o main.a

02.o: 02-reproducible.cpp
	g++ -std=c++14 -Wall -Wextra -O2 -g -fopenmp -pedantic 02-reproducible.cpp -o 02.o

01c.o: 01c-sir.cpp
	g++ -std=c++14 -Wall -Wextra -O2 -g -fopenmp -pedantic 01c-sir.cpp -o 01c.o



.PHONY: all
