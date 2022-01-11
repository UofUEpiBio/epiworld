VPATH = ../include/epiworld
CPPFLAGS= -std=c++11 -Wall -pedantic -g -O3 -ftree-vectorize -DEPIWORLD_TIMING # -fopenmp
GPP=g++
# icpx take a look at ~/.profile
# # /opt/intel/oneapi/setvars.sh 
INTEL=dpcpp
INTELFLAGS=-g -O2

00-hello-world.o: 00-hello-world.cpp
	$(GPP) $(CPPFLAGS) 00-hello-world.cpp -o 00-hello-world.o && ./00-hello-world.o

01-sir.o: 01-sir.cpp
	$(GPP) $(CPPFLAGS)  01-sir.cpp -o 01-sir.o && ./01-sir.o

02-sir_multiple_runs.o: 02-sir_multiple_runs.cpp
	$(GPP) $(CPPFLAGS)  02-sir_multiple_runs.cpp -o 02-sir_multiple_runs.o && ./02-sir_multiple_runs.o

02b-sir_multiple_runs.o: 02b-sir_multiple_runs.cpp
	$(GPP) $(CPPFLAGS)  02b-sir_multiple_runs.cpp -o 02b-sir_multiple_runs.o && ./02b-sir_multiple_runs.o

04-advanced-usage.o: 04-advanced-usage.cpp
	$(GPP) $(CPPFLAGS) 04-advanced-usage.cpp -o 04-advanced-usage.o && ./04-advanced-usage.o

05-user-data.o: 05-user-data.cpp
	$(GPP) $(CPPFLAGS) 05-user-data.cpp -o 05-user-data.o && ./05-user-data.o

06-sir-omp.o: 06-sir-omp.cpp
	$(GPP) $(CPPFLAGS) -fopenmp 06-sir-omp.cpp -o 06-sir-omp.o && ./06-sir-omp.o

07-surveillance.o: 07-surveillance.cpp
	$(GPP) $(CPPFLAGS)  07-surveillance.cpp -g -o 07-surveillance.o && ./07-surveillance.o 

# Others ----------------------------------------------------------------------
sir-intel.o: sir.cpp
	$(INTEL) $(INTELFLAGS)  sir.cpp -o sir-intel.o && ./sir-intel.o

sir-omp.o: sir.cpp
	$(INTEL)  $(INTELFLAGS) -qopenmp sir-omp.cpp -o sir-omp.o && ./sir-omp.o

ALL_EXAMPLES=00-hello-world.o 01-sir.o 02-sir_multiple_runs.o 02b-sir_multiple_runs.o \
	     04-advanced-usage.o 05-user-data.o 06-sir-omp.o 07-surveillance.o
all-examples:
	$(MAKE) $(ALL_EXAMPLES)

clean:
	rm -f *.o

mem-usage:
	valgrind --tool=massif ./sir.o && echo "Now use ms_print to see the memory usage"

.PHONY: clean check checkv mem-usage all-examples
