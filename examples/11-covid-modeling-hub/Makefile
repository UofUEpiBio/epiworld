NAME=main11
${NAME}.o: ${NAME}.cpp 
	g++ -std=c++11 -Wall -pedantic -g -O2 -ftree-vectorize ${NAME}.cpp -o ${NAME}.o
README.md: 08-queuing.o
	echo "## Example: 11 COVID Modeling Hub" > README.md && \
	echo "" >> README.md && \
	echo "Output from the program:" >> README.md && \
	echo "" >> README.md && \
	echo "\`\`\`" >> README.md && \
	./08-queuing.o >> README.md && \
	echo "\`\`\`" >> README.md
