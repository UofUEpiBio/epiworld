DIR_UP := $(shell cd .. && pwd)
CMD="podman"

build:
	${CMD} build -t gvegayon/epiworld:latest .

run:
	${CMD} run -i --rm -v $(DIR_UP):/home -w /home gvegayon/epiworld:latest

push:
	${CMD} push gvegayon/epiworld:latest
