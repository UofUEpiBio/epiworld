examples:
	cd examples && $(MAKE) -B
all-examples:
	cd examples && $(MAKE) all-examples

.PHONY: examples all-examples
