main.o: main.cpp
	g++ -std=c++11 -Wall -pedantic -g -mtune=native -O3 main.cpp -o main.o
main-intel.o: main.cpp
	icpc -std=c++11 -Wall -pedantic -g -O3 -mtune=native main.cpp -o main-intel.o
README.md: main.o
	echo "## Example: 01-sir" > README.md && \
	echo "" >> README.md && \
	echo "Output from the program:" >> README.md && \
	echo "" >> README.md && \
	echo "\`\`\`" >> README.md && \
	./main.o >> README.md && \
	echo "\`\`\`" >> README.md
