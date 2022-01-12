01-sir.o: 01-sir.cpp
	g++ -std=c++11 -Wall -pedantic -g -O3 -ftree-vectorize  01-sir.cpp -o 01-sir.o
README.md: 01-sir.o
	echo "## Example: 01-sir" > README.md && \
	echo "" >> README.md && \
	./01-sir.o >> README.md
