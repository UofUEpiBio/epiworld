main.o: main.cpp
	g++ -std=c++17 -Wall -pedantic -g -O2 -ftree-vectorize -fopenmp main.cpp -o main.o
README.md: main.o
	echo "## Example: 06b-sir-omp" > README.md && \
	echo "" >> README.md && \
	echo "Output from the program:" >> README.md && \
	echo "" >> README.md && \
	echo "\`\`\`" >> README.md && \
	./main.o >> README.md && \
	echo "\`\`\`" >> README.md
