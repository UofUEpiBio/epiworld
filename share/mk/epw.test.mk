include share/mk/epw.prog.mk

$(NAME)_TEST_DIR := $($(NAME)_BUILD_DIR)/.test
$(NAME)_COV_DIR := $($(NAME)_BUILD_DIR)/.coverage
	
# This calls into a Perl script, which:
# 
#   1. Runs the test suite with the -l flag to list all test cases.
#   2. For each test case, generates a Makefile rule that runs that test case.
#   3. Creates a target "all" that depends on all the test case rules.
# 
# The generated Makefile is then stored in the build directory as test.mk. This
# means that we can run tests in parallel using `make -j` and each test case will
# be run in its own rule. This also allows us to easily add new test cases without
# modifying the Makefile, as the test cases are discovered at build time.
$($(NAME)_BUILD_DIR)/test.mk: override NAME := $(NAME)
$($(NAME)_BUILD_DIR)/test.mk: $($(NAME)_BUILD_DIR)/$(NAME)
	$(SAY) "GEN" $@
	$(V)mkdir -p $($(NAME)_BUILD_DIR)
	$(V)perl script/test-gen-runner.pl '$(NAME)' '$($(NAME)_BUILD_DIR)' > $@
	
# Call into the generated test Makefile to run all tests.
# Then, if coverage is enabled, aggregate the coverage data.
.PHONY: $(NAME)-test
$(NAME)-test: override NAME := $(NAME)
$(NAME)-test: $(NAME)-test-gen-report

.PHONY: $(NAME)-test-gen-report
$(NAME)-test-gen-report: override NAME := $(NAME)
$(NAME)-test-gen-report: $(NAME)-tests
	$(SAY) "REPORT" $($(NAME)_TEST_DIR)/report.html
	$(V)perl $(ROOT_SOURCE_DIR)/script/junit-genhtml.pl $($(NAME)_TEST_DIR)/report.xml > $($(NAME)_TEST_DIR)/report.html
	$(SAY) "REPORT" $($(NAME)_TEST_DIR)/report.xml
	$(V)perl $(ROOT_SOURCE_DIR)/script/junit-okay.pl $($(NAME)_TEST_DIR)/report.xml; \
	JUNIT_STATUS=$$?; \
	if [ $$JUNIT_STATUS -eq 0 ]; then \
		perl $(ROOT_SOURCE_DIR)/script/junit-report.pl --short $($(NAME)_TEST_DIR)/report.xml; \
	else \
		perl $(ROOT_SOURCE_DIR)/script/junit-report.pl $($(NAME)_TEST_DIR)/report.xml; \
	fi; \
	exit $$JUNIT_STATUS
	
# With coverage on, every test process writes its own raw profile (%p is the
# process id); they are merged once the suite is done.
.PHONY: $(NAME)-tests
$(NAME)-tests: override NAME := $(NAME)
$(NAME)-tests: export LLVM_PROFILE_FILE := $(abspath $($(NAME)_COV_DIR))/%p.profraw
$(NAME)-tests: $($(NAME)_BUILD_DIR)/$(NAME) $($(NAME)_BUILD_DIR)/test.mk | $($(NAME)_TEST_HOOKS)
	$(SAY) "SUITE" $@
	$(V)mkdir -p $($(NAME)_TEST_DIR)
ifeq ($(WITH_COVERAGE),1)
	$(V)rm -rf $($(NAME)_COV_DIR) && mkdir -p $($(NAME)_COV_DIR)
endif
	
ifneq ($(TESTS),)
	$(V)echo "Running tests: $(TESTS)"
	$(V)mkdir -p $($(NAME)_TEST_DIR)
	$(V)cd $($(NAME)_TEST_DIR) && \
	IFS=';'; \
	set -- $(TESTS); \
	$(abspath $($(NAME)_BUILD_DIR)/$(NAME)) \
		--reporter junit \
		--out $(abspath $($(NAME)_TEST_DIR))/report.xml \
		"$$@"
else
	$(V)export OMP_NUM_THREADS=1; \
	$(MAKE) \
		-C $($(NAME)_TEST_DIR) \
		-f $(abspath $(ROOT_SOURCE_DIR))/$($(NAME)_BUILD_DIR)/test.mk \
		V='$(V)' SAY='$(SAY)'
        
	$(V)perl $(ROOT_SOURCE_DIR)/script/junit-combine.pl $($(NAME)_TEST_DIR)/report-*.xml > $(abspath $($(NAME)_TEST_DIR))/report.xml
endif

ifeq ($(WITH_COVERAGE),1)
	$(SAY) "COV" $($(NAME)_COV_DIR)/coverage.info
	$(V)$(LLVM_PROFDATA) merge -sparse $($(NAME)_COV_DIR)/*.profraw -o $($(NAME)_COV_DIR)/$(NAME).profdata
	$(V)$(LLVM_COV) export -format=lcov -instr-profile $($(NAME)_COV_DIR)/$(NAME).profdata \
		$($(NAME)_BUILD_DIR)/$(NAME) $(abspath $($(NAME)_COV_DIRS)) > $($(NAME)_COV_DIR)/coverage.info
endif

TEST_TARGETS += $(NAME)-test	

TEST_FILES += $(addprefix $($(NAME)_SOURCE_DIR)/,$($(NAME)_SOURCES))
$(foreach src,$(addprefix $($(NAME)_SOURCE_DIR)/,$($(NAME)_SOURCES)), \
  $(eval $(src)_HAS_TESTS := \
     $(shell cat $(src) | perl $(ROOT_SOURCE_DIR)/script/test-list-defined.pl)) \
)
