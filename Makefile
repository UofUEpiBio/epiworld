examples:
	cd examples && $(MAKE) -B
all-examples:
	cd examples && $(MAKE) all-examples

epiworld.hpp:
	Rscript --vanilla --verbose epiworld-hpp.R

readme.o: readme.cpp epiworld.hpp
	g++ -std=c++17 -O2 readme.cpp -o readme.o

helloworld.o: helloworld.cpp epiworld.hpp
	g++ -std=c++17 -O2 helloworld.cpp -o helloworld.o

.PHONY: examples all-examples epiworld.hpp
