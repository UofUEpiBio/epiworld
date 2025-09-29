ifndef CXX_STD
	CXX_STD := c++23
endif

CXX       := g++
CLANG     := clang++
CXXFLAGS  := -std=$(CXX_STD) -Wall -Wextra -O0 -g -pedantic
OPTFLAGS  := -O2 -g
OPENMP    := $(OPENMP)

# Default target
all: main.o

# Generic rule: .cpp -> .o (actually executable here, not object file)
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) $(OPENMP) -Wnull-dereference \
		-fdelete-null-pointer-checks -pedantic $< -o $@

# Special cases
main.a: main.cpp
	$(CLANG) $(CXXFLAGS) $(OPTFLAGS) $(OPENMP) -Wall -Wextra -Wpedantic $< -o $@

callgrind: 05-mixing-with-size.o
	rm -f callgrind.out.* && \
	valgrind --tool=callgrind ./05-mixing-with-size.o && \
	callgrind_annotate callgrind.out.* > callgrind_report.txt

coverage: clean
	$(CXX) $(CXXFLAGS) -O2 -g -fprofile-arcs -ftest-coverage main.cpp -o main.o && \
	./main.o && \
	mv main.o-main.gcda main.gcda && \
	mv main.o-main.gcno main.gcno && \
	gcov -r main.cpp && ls -allt && \
	rm -f catch*.hpp.gcov *cpp.gcov

clean:
	rm -f *.o *.a *.so *.out *.exe *.gcda *.gcno *.gcov
	rm -fr *-saves/
	for file in *.cpp; do \
		mkdir -p "$${file%.cpp}-saves"; \
	done

.PHONY: all clean coverage callgrind
