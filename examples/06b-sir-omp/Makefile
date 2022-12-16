main.o: main.cpp
	g++ -std=c++11 -Wall -pedantic -g -O2 -ftree-vectorize -fopenmp main.cpp -o main.o
README.md: main.o
	echo "## Example: 06b-sir-omp" > README.md && \
	echo "" >> README.md && \
	echo "Output from the program:" >> README.md && \
	echo "" >> README.md && \
	echo "\`\`\`" >> README.md && \
	./main.o 2 20 >> README.md && \
	echo "\`\`\`" >> README.md
