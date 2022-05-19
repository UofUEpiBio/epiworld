examples:
	cd examples && $(MAKE) -B
all-examples:
	cd examples && $(MAKE) all-examples

epiworld.hpp:
	Rscript --vanilla --verbose epiworld-hpp.R

.PHONY: examples all-examples epiworld.hpp
