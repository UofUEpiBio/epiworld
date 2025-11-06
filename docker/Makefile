DOCKER_IMG ?= gvegayon/epiworld:latest
DOCKER_CMD ?= podman

docker-build:
	$(DOCKER_CMD) build -t $(DOCKER_IMG) $(ROOT_SOURCE_DIR)/docker

docker-run:
	$(DOCKER_CMD) run -i --rm -v $(abspath $(ROOT_SOURCE_DIR)):/home -w /home $(DOCKER_IMG)

docker-push:
	$(DOCKER_CMD) push $(DOCKER_IMG)
