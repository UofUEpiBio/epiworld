all: main.o

main.o: main.cpp clean
	g++ -std=c++14 -Wunused -Wall -Wextra -Wnull-dereference -fdelete-null-pointer-checks -O2 $(OPENMP) -g -pedantic main.cpp -o main.o

main.a: main.cpp clean
	clang++ -std=c++14 -Wall -Wextra $(OPENMP) -Wpedantic -O2 main.cpp -o main.a

00-lfmcmc.o: 00-lfmcmc.cpp
	g++ -std=c++14 -Wall -Wextra -O2 -g $(OPENMP) -pedantic 00-lfmcmc.cpp -o 00-lfmcmc.o

# Compile 02-reproducible-sirconn.cpp
02-reproducible-sirconn.o: 02-reproducible-sirconn.cpp
	g++ -std=c++14 -Wall -Wextra -O2 -g $(OPENMP) -pedantic 02-reproducible-sirconn.cpp -o 02-reproducible-sirconn.o

02.o: 02-reproducible.cpp
	g++ -std=c++14 -Wall -Wextra -O2 -g $(OPENMP) -pedantic 02-reproducible.cpp -o 02.o

01c.o: 01c-sir.cpp
	g++ -std=c++14 -Wall -Wextra -O2 -g $(OPENMP) -pedantic 01c-sir.cpp -o 01c.o

05-mixing.o: 05-mixing.cpp
	g++ -std=c++14 -Wall -Wextra -O2 -g $(OPENMP) -pedantic 05-mixing.cpp -o 05-mixing.o

# Check coverage using the main.o target
coverage: main.o
	gcov -r main.cpp && ls -allt && \
	rm -f catch*.hpp.gcov && \
	mkdir -p gcov-files && rm -f gcov-files/* && \
	mv *.hpp.gcov gcov-files/ && rm -f *.gcov


# Create folders named after the *.cpp files in the current directory
# and append "-saves" to the folder name
# e.g. 01-sir.cpp -> 01-sir-saves
clean:
	rm -fr *-saves/
	for file in *.cpp; do \
		mkdir -p "$${file%.cpp}-saves"; \
	done



# What's the -ftest-coverage flag?
# https://stackoverflow.com/questions/9842759/what-is-ftest-coverage-option-in-gcov



.PHONY: all clean coverage
