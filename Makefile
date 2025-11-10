help:
	@echo "Makefile targets:"
	@echo "  examples          - Build all example programs"
	@echo "  all-examples     - Build all example programs (force rebuild)"
	@echo "  epiworld.hpp     - Generate the epiworld.hpp header file"
	@echo "  docs             - Build the documentation site"
	@echo "  docs-serve       - Serve the documentation site locally"
	@echo "  docs-clean       - Clean the generated documentation site"
	@echo "  oneapi           - Start a Docker container with Intel oneAPI environment"
	@echo "  diagrams         - Generate diagrams from Mermaid files"

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

# Documentation targets
docs: docs-mkdocs

docs-mkdocs:
	mkdocs build

docs-serve:
	mkdocs serve

docs-clean:
	rm -rf site

.PHONY: examples all-examples epiworld.hpp docs docs-mkdocs docs-serve docs-clean

image=intel/oneapi-basekit:devel-ubuntu22.04
# --device=/dev/dri enables the gpu (if available). May not be available in Linux VM or Windows
oneapi:
	docker run --cap-add=SYS_ADMIN --cap-add=SYS_PTRACE \
		--device=/dev/dri -it "${image}"

diagrams:
	cd include/epiworld/models && \
	for file in *.mmd; do \
		echo "Processing $$file..."; \
		cat "$$file" | base64 -w0 | xargs -I {} curl -s "https://mermaid.ink/img/{}" > "$${file%.mmd}.png"; \
	done && \
	mv *.png ../../../docs_src/assets/img/