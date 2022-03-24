09-sir-connected.o: 09-sir-connected.cpp
	g++ -std=c++17 -Wall -pedantic -g -O2 -ftree-vectorize  09-sir-connected.cpp -o 09-sir-connected.o
09-seir-connected.o: 09-seir-connected.cpp
	g++ -std=c++17 -Wall -pedantic -g -O2 -ftree-vectorize  09-seir-connected.cpp -o 09-seir-connected.o

README.md: 09-sir-connected.o
	echo "## Example: 09-sir-connected" > README.md && \
	echo "" >> README.md && \
	echo "Output from the program:" >> README.md && \
	echo "" >> README.md && \
	echo "\`\`\`" >> README.md && \
	./09-sir-connected.o >> README.md && \
	echo "\`\`\`" >> README.md
