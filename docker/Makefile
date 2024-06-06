DIR_UP := $(shell cd .. && pwd)

build:
	docker build -t gvegayon/epiworld:latest .

run:
	docker run -i --rm -v $(DIR_UP):/home -w /home gvegayon/epiworld:latest

push:
	docker push gvegayon/epiworld:latest
