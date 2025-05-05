main.o: main.cpp
	g++ -std=c++17 -Wall -pedantic -g -O2 -ftree-vectorize  main.cpp -o main.o
README.md: main.o
	echo "## Example: 01-sir" > README.md && \
	echo "" >> README.md && \
	echo "Output from the program:" >> README.md && \
	echo "" >> README.md && \
	echo "\`\`\`" >> README.md && \
	./main.o >> README.md && \
	echo "\`\`\`" >> README.md
