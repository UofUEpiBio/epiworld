00-hello-world.o: 00-hello-world.cpp
	g++ -std=c++11 -Wall -pedantic -g -O3 -ftree-vectorize  00-hello-world.cpp -o 00-hello-world.o
README.md: 00-hello-world.o
	echo "## Example: 00-hello-world" > README.md && \
	echo "" >> README.md && \
	./00-hello-world.o >> README.md
