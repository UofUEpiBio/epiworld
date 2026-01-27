$(NAME)_SOURCES := \
	main.cpp \
   	00-cloning-model.cpp \
	00-lfmcmc.cpp \
	01-model-methods.cpp \
	01-sample.cpp \
	01-sir.cpp \
	01-sirconnected.cpp \
	01b-sir.cpp \
	01c-sir.cpp \
	02-reproducible-sir.cpp \
	02-reproducible-sirconn.cpp \
	03-sis.cpp \
	04-initial-dist.cpp \
	05-mixing.cpp \
	05b-mixing.cpp \
	05c-mixing.cpp \
	06-mixing.cpp \
	07-entitifuns.cpp \
	09-distribute-tools-and-viruses.cpp \
	10-generation-interval.cpp \
	11-random-numbers.cpp \
	12-diagrams.cpp \
	13-rt.cpp \
	14a-measles.cpp \
	14b-measles.cpp \
	14c-measles.cpp \
	14d-measles.cpp \
	15-edgelist-after-run-multiple.cpp \
	16-multiple-events.cpp \
	17a-mixing-quarantine.cpp \
	17b-mixing-quarantine.cpp \
	17c-mixing-quarantine.cpp \
	18a-measles-mixing.cpp \
	18b-measles-mixing.cpp \
	18c-measles-mixing.cpp \
	19a-measles-mixing-risk-quarantine-basic.cpp \
	19b-measles-mixing-risk-quarantine-transitions.cpp \
	19c-measles-mixing-risk-quarantine-durations.cpp \
	19d-measles-mixing-risk-quarantine-process.cpp \
	20a-rewire-degseq.cpp \
	20b-rewire-degseq.cpp \
	22a-globalevents.cpp \
	22b-globalevents.cpp \
	23a-distribute-tool-virus-to-entities.cpp \
	23b-distribute-tool-virus-to-entities.cpp \
	24a-virus-hist-single-run.cpp \
	24b-csv-output-format.cpp \
	24c-virus-hist-multiple-sims.cpp \
	25a-hospitalizationstracker.cpp \
	25b-hospitalizationstracker-validation.cpp


$(NAME)_CXXFLAGS := \
	-O2

$(NAME)_COV_DIRS := \
	$($(NAME)_SOURCE_DIR) \
	$(ROOT_SOURCE_DIR)/include/epiworld
	
$(NAME)_TEST_HOOKS := \
	make-all-$(NAME)-dirs \
	relocate-all-$(NAME)-resources

include share/mk/epw.test.mk
	
# HOOK: Make all saver dirs needed for tests.
.PHONY: make-all-$(NAME)-dirs
make-all-$(NAME)-dirs: override NAME := $(NAME)
make-all-$(NAME)-dirs:
	$(SAY) "HOOK" $@
	$(V) mkdir -p $($(NAME)_TEST_DIR)
	$(V)for f in "$($(NAME)_SOURCE_DIR)"/*.cpp; do \
	    mkdir -p "$($(NAME)_TEST_DIR)/$$(basename "$$f" .cpp)-saves"; \
	done
	
# HOOK: Relocate all resource files to the test directory.
.PHONY: relocate-all-$(NAME)-resources
relocate-all-$(NAME)-resources: override NAME := $(NAME)
relocate-all-$(NAME)-resources:
	$(SAY) "HOOK" $@
	$(V) mkdir -p $($(NAME)_TEST_DIR)
	$(V) cp $($(NAME)_SOURCE_DIR)/resources/*  $($(NAME)_TEST_DIR)/
