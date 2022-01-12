05-user-data.o: 05-user-data.cpp
	g++ -std=c++11 -Wall -pedantic -g -O2 -ftree-vectorize  05-user-data.cpp -o 05-user-data.o
README.md: 05-user-data.o
	echo "## Example: 05-user-data" > README.md && \
	echo "" >> README.md && \
	echo "Output from the program:" >> README.md && \
	echo "" >> README.md && \
	echo "\`\`\`" >> README.md && \
	./05-user-data.o >> README.md && \
	echo "\`\`\`" >> README.md
