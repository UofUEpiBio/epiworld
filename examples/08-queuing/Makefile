08-queuing.o: 08-queuing.cpp
	g++ -std=c++11 -Wall -pedantic -g -O2 -ftree-vectorize 08-queuing.cpp -o 08-queuing.o
README.md: 08-queuing.o
	echo "## Example: 08-queuing" > README.md && \
	echo "" >> README.md && \
	echo "Output from the program:" >> README.md && \
	echo "" >> README.md && \
	echo "\`\`\`" >> README.md && \
	./08-queuing.o >> README.md && \
	echo "\`\`\`" >> README.md
