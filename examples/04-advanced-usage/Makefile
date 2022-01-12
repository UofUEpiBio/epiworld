04-advanced-usage.o: 04-advanced-usage.cpp
	g++ -std=c++11 -Wall -pedantic -g -O2 -ftree-vectorize  04-advanced-usage.cpp -o 04-advanced-usage.o
README.md: 04-advanced-usage.o
	echo "## Example: 04-advanced-usage" > README.md && \
	echo "" >> README.md && \
	echo "Output from the program:" >> README.md && \
	echo "" >> README.md && \
	echo "\`\`\`" >> README.md && \
	./04-advanced-usage.o >> README.md && \
	echo "\`\`\`" >> README.md
