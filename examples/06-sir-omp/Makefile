06-sir-omp.o: 06-sir-omp.cpp
	g++ -std=c++11 -Wall -pedantic -g -O3 -fopenmp -ftree-vectorize  06-sir-omp.cpp -o 06-sir-omp.o
README.md: 06-sir-omp.o
	echo "## Example: 06-sir-omp" > README.md && \
	echo "" >> README.md && \
	./06-sir-omp.o >> README.md
