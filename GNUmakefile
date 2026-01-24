# This GNU Make system was written by Olivia Banks to build a variety of projects,
# and is now used to build Epiworld as well. It was difficult to write, but it
# should be easy to use; if you use this in your own projects, please credit the
# original author. Thanks!

include share/mk/epw.util.mk

# Makefile settings.
.SUFFIXES:
.DEFAULT_GOAL := all

# Critical tooling.
CC  ?= cc
CXX ?= c++

# Non-critical tooling.
DOXYGEN            ?= doxygen
MKDOCS             ?= mkdocs
LCOV               ?= lcov
VALGRIND           ?= valgrind
CALLGRIND_ANNOTATE ?= callgrind_annotate

# Control flags.

# Verbosity.
V := $(if $(VERBOSE),,@)

# Run tests in parallel.
PARALLEL_TESTS ?= 1

# Compilation flags.

# Build profile: debug or release.
BUILD_PROFILE ?= debug

# Enable OpenMP support.
WITH_OPENMP   ?= 1

# Enable code coverage support.
WITH_COVERAGE ?= 0

CFLAGS   :=
CXXFLAGS :=
LDFLAGS  :=

ifeq ($(BUILD_PROFILE),debug)
    CFLAGS += -g -O0 -DDEBUG -Wno-unused-parameter -fno-omit-frame-pointer -fstack-protector-all -Wall -Wextra
    CXXFLAGS += -g -O0 -DDEBUG -Wno-unused-parameter -fno-omit-frame-pointer -fstack-protector-all -Wall -Wextra
else ifeq ($(BUILD_PROFILE),release)
    CFLAGS += -O3 -DNDEBUG -Wno-unused-parameter -ftree-vectorize -funroll-loops -ffast-math -march=native
    CXXFLAGS += -O3 -DNDEBUG -Wno-unused-parameter -ftree-vectorize -funroll-loops -ffast-math -march=native 
else
    $(error "Unknown BUILD_PROFILE: '$(BUILD_PROFILE)'. Valid options are 'debug' and 'release'.")
endif

ifeq ($(WITH_OPENMP),1)
    CFLAGS   += -fopenmp -fopenmp=libomp
    CXXFLAGS += -fopenmp -fopenmp=libomp
    LDFLAGS  += -fopenmp -fopenmp=libomp
endif

ifeq ($(WITH_COVERAGE),1)
    CFLAGS   += --coverage
    CXXFLAGS += --coverage
    LDFLAGS  += --coverage
endif

# Package tracking.
DIRECTORY_STACK  := .
ROOT_SOURCE_DIR  := .
ROOT_BUILD_DIR   := $(ROOT_SOURCE_DIR)/build

# Target tracking.
ALL_PROGRAMS     :=
TEST_FILES       :=

EXAMPLE_TARGETS  :=
README_TARGETS   :=
TEST_TARGETS	 :=

# What packages to include.
ALL_PACKAGES := examples tests 
PACKAGES     := $(ALL_PACKAGES)

# Helper rules.
.PHONY: purge
purge:
	$(SAY) "RM" $(ROOT_BUILD_DIR)
	$(V)rm -rf $(ROOT_BUILD_DIR)

.PHONY: docs
docs:
	$(error "unimplemented")

.PHONY: help
help: all
	
	
# Recursive package expansion.
define INCLUDE_PACKAGE
	PACKAGES :=
	DIRECTORY_STACK := $$(call PUSH,$$(DIRECTORY_STACK),$(1))
	
  	NAME := $(1)
	$$(NAME)_SOURCE_DIR := $$(call STACK_TO_PATH,$$(DIRECTORY_STACK))
	$$(NAME)_BUILD_DIR := $$(subst $(ROOT_SOURCE_DIR),$(ROOT_BUILD_DIR),$$($$(NAME)_SOURCE_DIR))

	include $$($$(NAME)_SOURCE_DIR)/Makefile
    $$(foreach package,$$(PACKAGES),$$(eval $$(call INCLUDE_PACKAGE,$$(package))))

    $(if $(VERBOSE),$$(info Included package: '$$(NAME)' from '$$($$(NAME)_SOURCE_DIR)'))
    DIRECTORY_STACK := $$(call POP,$$(DIRECTORY_STACK))
endef

# Build packages.
$(foreach package,$(PACKAGES),$(eval $(call INCLUDE_PACKAGE,$(package))))

# And this is where it all begins...
.PHONY: examples
examples: $(EXAMPLE_TARGETS)
	
.PHONY: readmes
readmes: $(README_TARGETS)

.PHONY: test
test: $(TEST_TARGETS)

.PHONY: tests
tests:
	$(error cannot build tests separately, yet)

# Aliases
# And this is where it all begins...
.PHONY: example
example: examples
	
.PHONY: readme
readme: readmes

.PHONY: all
all:
	@printf "To build a specific target, run:\n"	
	@printf "\n"
	@printf "\t$(MAKE) <target>\n"
	@printf "\n"
	@printf "See the README for project documentation.\n"

# Emit our internal databases of what we know about.
.PHONY: program-database
program-database:
	$(V)$(foreach program,$(ALL_PROGRAMS),printf "D %s: %s\n" $(program) '$($(program)_DISPATCHES)';)
	
.PHONY: test-database
test-database:
	$(V)$(foreach test,$(TEST_FILES),printf "T %s: %s\n" $(test) '$($(test)_HAS_TESTS)';)
	
# While not actually phony, we should always rebuild the amalgam when requested.
.PHONY: $(ROOT_BUILD_DIR)/epiworld.hpp
$(ROOT_BUILD_DIR)/epiworld.hpp: $(ROOT_SOURCE_DIR)/script/amalgamate.pl
	$(SAY) "PERL" $@
	@mkdir -p $(ROOT_BUILD_DIR)
	$(V)perl $(ROOT_SOURCE_DIR)/script/amalgamate.pl $(ROOT_SOURCE_DIR)/include/epiworld epiworld.hpp \
	    > $(ROOT_BUILD_DIR)/epiworld.hpp

# TODO: I haven't seen a good reason why we should clean each artifact instead of just nuking
#  the build directory.
.PHONY: clean
clean: purge
	@printf "Consider using '$(MAKE) purge' instead.\n"

include docker/Makefile
