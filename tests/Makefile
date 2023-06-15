all: main.o

main.o: main.cpp
	g++ -std=c++14 -Wall -Wextra -Wnull-dereference -fdelete-null-pointer-checks -O2 -fopenmp -g -pedantic main.cpp -o main.o

main.a: main.cpp
	clang++ -std=c++14 -Wall -Wextra -fopenmp -Wpedantic -O2 main.cpp -o main.a

# Compile 02-reproducible-sirconn.cpp
02-reproducible-sirconn.o: 02-reproducible-sirconn.cpp
	g++ -std=c++14 -Wall -Wextra -O2 -g -fopenmp -pedantic 02-reproducible-sirconn.cpp -o 02-reproducible-sirconn.o

02.o: 02-reproducible.cpp
	g++ -std=c++14 -Wall -Wextra -O2 -g -fopenmp -pedantic 02-reproducible.cpp -o 02.o

01c.o: 01c-sir.cpp
	g++ -std=c++14 -Wall -Wextra -O2 -g -fopenmp -pedantic 01c-sir.cpp -o 01c.o

# Check coverage using the main.o target
coverage: main.o
	g++ -g -O1 -fprofile-arcs -ftest-coverage main.cpp -o main.o && \
		./main.o && gcov -r main.cpp && \
		rm catch*.hpp.gcov && \
		mv *.hpp.gcov gcov-files/ && $(MAKE) clean


# What's the -ftest-coverage flag?
# https://stackoverflow.com/questions/9842759/what-is-ftest-coverage-option-in-gcov



.PHONY: all
