02-sir_multiple_runs.o: 02-sir_multiple_runs.cpp
	g++ -std=c++11 -Wall -pedantic -g -O3 -ftree-vectorize  02-sir_multiple_runs.cpp -o 02-sir_multiple_runs.o
README.md: 02-sir_multiple_runs.o
	echo "## Example: 02-sir_multiple_runs" > README.md && \
	echo "" >> README.md && \
	./02-sir_multiple_runs.o >> README.md
