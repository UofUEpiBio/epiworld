07-surveillance.o: 07-surveillance.cpp
	g++ -std=c++11 -Wall -pedantic -g -O3 -ftree-vectorize  07-surveillance.cpp -o 07-surveillance.o
README.md: 07-surveillance.o
	echo "## Example: 07-surveillance" > README.md && \
	echo "" >> README.md && \
	./07-surveillance.o >> README.md
