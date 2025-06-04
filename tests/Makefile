
CXX_FLAGS_DEBUG=-std=c++23 -Wall -Wextra -O0 -g -pedantic -fopenmp

ifndef CXX_STD
	CXX_STD=-std=c++23
endif

all: main.o

main.o: main.cpp clean
	g++ $(CXX_STD) -Wunused -Wall -Wextra -Wnull-dereference -fdelete-null-pointer-checks -O2 $(OPENMP) -g -pedantic main.cpp -o main.o

main.a: main.cpp clean
	clang++ $(CXX_STD) -Wall -Wextra $(OPENMP) -Wpedantic -O2 main.cpp -o main.a

00-lfmcmc.o: 00-lfmcmc.cpp
	g++ $(CXX_STD) -Wall -Wextra -O2 -g $(OPENMP) -pedantic 00-lfmcmc.cpp -o 00-lfmcmc.o

# Compile 02-reproducible-sirconn.cpp
02-reproducible-sirconn.o: 02-reproducible-sirconn.cpp
	g++ $(CXX_STD) -Wall -Wextra -O2 -g $(OPENMP) -pedantic 02-reproducible-sirconn.cpp -o 02-reproducible-sirconn.o

02.o: 02-reproducible.cpp
	g++ $(CXX_STD) -Wall -Wextra -O2 -g $(OPENMP) -pedantic 02-reproducible.cpp -o 02.o

01c.o: 01c-sir.cpp
	g++ $(CXX_STD) -Wall -Wextra -O2 -g $(OPENMP) -pedantic 01c-sir.cpp -o 01c.o

05-mixing.o: 05-mixing.cpp
	g++ $(CXX_STD) -Wall -Wextra -O2 -g $(OPENMP) -pedantic 05-mixing.cpp -o 05-mixing.o

# Check coverage using the main.o target
coverage: clean
	g++ $(CXX_STD) -O2 -g -fprofile-arcs -ftest-coverage main.cpp -o main.o && \
	./main.o && \
	mv main.o-main.gcda main.gcda && \
	mv main.o-main.gcno main.gcno && \
	gcov -r main.cpp && ls -allt && \
	rm -f catch*.hpp.gcov *cpp.gcov # && \
	# mkdir -p gcov-files && rm -f gcov-files/* && \
	# mv *.hpp.gcov gcov-files/ && rm -f *.gcov


# Create folders named after the *.cpp files in the current directory
# and append "-saves" to the folder name
# e.g. 01-sir.cpp -> 01-sir-saves
clean:
	rm -f *.o *.a *.so *.out *.exe *.gcda *.gcno *.gcov
	rm -fr *-saves/
	for file in *.cpp; do \
		mkdir -p "$${file%.cpp}-saves"; \
	done

# What's the -ftest-coverage flag?
# https://stackoverflow.com/questions/9842759/what-is-ftest-coverage-option-in-gcov

.PHONY: all clean coverage

05-mixing-with-size.o: 05-mixing-with-size.cpp
	g++ $(CXX_FLAGS_DEBUG) 05-mixing-with-size.cpp -o 05-mixing-with-size.o

callgrind: 05-mixing-with-size.cpp
	rm -f callgrind.out.* && \
	g++ $(CXX_FLAGS_DEBUG) 05-mixing-with-size.cpp -o 05-mixing-with-size.o && \
	valgrind --tool=callgrind ./05-mixing-with-size.o && \
	callgrind_annotate callgrind.out.* > callgrind_report.txt


05b-mixing.o: 05b-mixing.cpp
	g++ $(CXX_STD) -Wall -Wextra -O2 -g $(OPENMP) -pedantic 05b-mixing.cpp \
		-o 05b-mixing.o
