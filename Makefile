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

.PHONY: examples all-examples epiworld.hpp format oneapi

image=intel/oneapi-basekit:devel-ubuntu22.04
# --device=/dev/dri enables the gpu (if available). May not be available in Linux VM or Windows
oneapi:
	docker run --cap-add=SYS_ADMIN --cap-add=SYS_PTRACE \
		--device=/dev/dri -it "${image}"

format:
	find . -iname '*.h' -o -iname '*.cpp' -o -iname '*.hpp' | clang-format --style=file -i --files=/dev/stdin
