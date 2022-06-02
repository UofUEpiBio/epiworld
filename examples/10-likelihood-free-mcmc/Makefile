10-likelihood-free-mcmc.o: 10-likelihood-free-mcmc.cpp
	g++ -std=c++11 -Wall -pedantic -g -O2 -ftree-vectorize 10-likelihood-free-mcmc.cpp -o 10-likelihood-free-mcmc.o
README.md: 10-likelihood-free-mcmc.o
	echo "## Example: 10-likelihood-free-mcmc" > README.md && \
	echo "" >> README.md && \
	echo "Output from the program:" >> README.md && \
	echo "" >> README.md && \
	echo "\`\`\`" >> README.md && \
	./10-likelihood-free-mcmc.o >> README.md && \
	echo "\`\`\`" >> README.md
