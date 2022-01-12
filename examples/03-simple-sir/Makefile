03-simple-sir.o: 03-simple-sir.cpp
	g++ -std=c++11 -Wall -pedantic -g -O2 -ftree-vectorize  03-simple-sir.cpp -o 03-simple-sir.o
README.md: 03-simple-sir.o
	echo "## Example: 03-simple-sir" > README.md && \
	echo "" >> README.md && \
	echo "Output from the program:" >> README.md && \
	echo "" >> README.md && \
	echo "\`\`\`" >> README.md && \
	./03-simple-sir.o >> README.md && \
	echo "\`\`\`" >> README.md
