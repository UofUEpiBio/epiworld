main.o: main.cpp
	g++ -std=c++17 -Wall -pedantic -g -O3 -ftree-vectorize  -fopenmp main.cpp -o main.o
README.md: main.o
	echo "## Example: 02b-sir_multiple_runs" > README.md && \
	echo "" >> README.md && \
	echo "Output from the program:" >> README.md && \
	echo "" >> README.md && \
	echo "\`\`\`" >> README.md && \
	./main.o >> README.md && \
	echo "\`\`\`" >> README.md
