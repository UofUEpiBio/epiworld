# Segfaulting (#116):
# 	01b-sir.cpp \
# 	01c-sir.cpp \
#   02-reproducible-sir.cpp \
#   02-reproducible-sirconn.cpp \
# 	04-initial-dist.cpp \
# 	05b-mixing.cpp \
# 	05c-mixing.cpp \
# 	07-entitifuns.cpp \
# 	10-generation-interval.cpp \
# 	12-diagrams.cpp \
# 	15-edgelist-after-run-multiple.cpp \
# 	16-multiple-events.cpp \
# 	18a-measles-mixing.cpp \
# 	18b-measles-mixing.cpp \
# 	18c-measles-mixing.cpp \
# 	19a-measles-mixing-risk-quarantine-basic.cpp \
#	19b-measles-mixing-risk-quarantine-transitions.cpp \
#	19c-measles-mixing-risk-quarantine-durations.cpp \
#	19d-measles-mixing-risk-quarantine-process.cpp \
# 
# Failing (#116):
# 	05-mixing.cpp \
#   06-mixing.cpp \
#   09-distribution-tools-and-viruses.cpp \
#   13-rt.cpp \
#   14a-measles.cpp \
#   14b-measles.cpp \
#   14c-measles.cpp \
#   17a-mixing-quarantine.cpp \
#   17b-mixing-quarantine.cpp \
#   17c-mixing-quarantine.cpp \

$(NAME)_SOURCES := \
	main.cpp \
	00-cloning-model.cpp \
	00-lfmcmc.cpp \
	01-model-methods.cpp \
	01-sir.cpp \
	01-sirconnected.cpp \
	11-random-numbers.cpp \
	17a-mixing-quarantine.cpp \
	20a-rewire-degseq.cpp \
	20b-rewire-degseq.cpp
	
$(NAME)_TEST_HOOKS := \
	relocate-all-$(NAME)-resources

include share/mk/epw.test.mk
	
# HOOK: Relocate all resource files to the test directory.
.PHONY: relocate-all-$(NAME)-resources
relocate-all-$(NAME)-resources: override NAME := $(NAME)
relocate-all-$(NAME)-resources:
	$(SAY) "HOOK" $@
	$(V) mkdir -p $($(NAME)_TEST_DIR)
	$(V) cp $($(NAME)_SOURCE_DIR)/resources/*  $($(NAME)_TEST_DIR)/
