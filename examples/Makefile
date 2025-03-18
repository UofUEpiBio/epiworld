VPATH = ../include/epiworld
CPPFLAGS= -std=c++14 -Wall -pedantic -g -O2 -ftree-vectorize $(OPENMP)
GPP=g++
# icpx take a look at ~/.profile
# # /opt/intel/oneapi/setvars.sh 
INTEL=dpcpp
INTELFLAGS=-g -O2

ALL_EXAMPLES=00-hello-world 01-sir 02-sir_multiple_runs 02b-sir_multiple_runs \
	     03-simple-sir 04-advanced-usage 05-user-data 06-sir-omp 06b-sir-omp \
	     07-surveillance 10-likelihood-free-mcmc 11-entities 13-genint \
		 14-community-hosp

all-examples:
	for f in $(ALL_EXAMPLES); do \
		cd $$f && $(MAKE) main.o && ./main.o && cd .. ;\
	done

all-readme:
	for f in $(ALL_EXAMPLES); do \
		cd $$f && $(MAKE) README.md ; cd .. ;\
	done

# Others ----------------------------------------------------------------------
00-hello-world/hello-intel.o: 00-hello-world/00-hello-world.cpp
	cd 00-hello-world && $(INTEL) $(INTELFLAGS) 00-hello-world.cpp -o hello-intel.o && ./hello-intel.o

sir-omp.o: sir.cpp
	$(INTEL)  $(INTELFLAGS) -qopenmp sir-omp.cpp -o sir-omp.o && ./sir-omp.o

allmake:
	for f in $(ALL_EXAMPLES); do \
		mkdir $$f ; \
		mv $$f.* $$f ;\
		echo "$$f.o: $$f.cpp\n\t$(GPP) $(CPPFLAGS) $$f.cpp -o $$f.o" > $$f/Makefile ; \
		echo "README.md: $$f.o" >> $$f/Makefile ; \
		echo "\techo \"## Example: $$f\" > README.md && \\" >> $$f/Makefile ; \
		echo "\techo \"\" >> README.md && \\"               >> $$f/Makefile ; \
		echo "\techo \"Output from the program:\" >> README.md && \\" >> $$f/Makefile ; \
		echo "\techo \"\" >> README.md && \\" >> $$f/Makefile ; \
		echo "\techo \"\\\`\\\`\\\`\" >> README.md && \\" >> $$f/Makefile ; \
		echo "\t./$$f.o >> README.md && \\" >> $$f/Makefile ; \
		echo "\techo \"\\\`\\\`\\\`\" >> README.md" >> $$f/Makefile ; \
	done

clean:
	rm -f */*.o

mem-usage:
	valgrind --tool=massif ./sir.o && echo "Now use ms_print to see the memory usage"

.PHONY: clean check checkv mem-usage all-examples
