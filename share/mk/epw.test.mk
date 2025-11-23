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
	$(V)perl script/gen-test-runner.pl '$(NAME)' '$($(NAME)_BUILD_DIR)' $($(NAME)_COV_DIRS) > $@

# Loop over all test suites and add a target to run them.
define run-test-rule
$(NAME)-$(basename $(1)): $($(NAME)_BUILD_DIR)/test.mk
	$(SAY) "TEST" $(basename $(1))
	$(V)$(MAKE) \
        -C $($(NAME)_TEST_DIR) \
        -f $(abspath $(ROOT_SOURCE_DIR))/$($(NAME)_BUILD_DIR)/test.mk $(MAKEFLAGS) \
        V='$(V)' SAY='$(SAY)' WITH_COVERAGE='$(WITH_COVERAGE)' LCOV='$(LCOV)'\
        $(shell printf '$(basename $(1))' | sha256sum | cut -d' ' -f1)

 
endef

$(eval $(foreach src,$($(NAME)_SOURCES),$(call run-test-rule,$(src))))

# Call into the generated test Makefile to run all tests.
# Then, if coverage is enabled, aggregate the coverage data.
.PHONY: $(NAME)-test
$(NAME)-test: override NAME := $(NAME)
$(NAME)-test: $(NAME)-test-gen-report

.PHONY: $(NAME)-test-gen-report
$(NAME)-test-gen-report: override NAME := $(NAME)
$(NAME)-test-gen-report: $(NAME)-tests-all
	$(SAY) "REPORT" $($(NAME)_TEST_DIR)/report.html
	$(V)perl $(ROOT_SOURCE_DIR)/script/junit-genhtml.pl $($(NAME)_TEST_DIR)/report.xml > $($(NAME)_TEST_DIR)/report.html
	$(SAY) "REPORT" $($(NAME)_TEST_DIR)/report.xml
	$(V)perl $(ROOT_SOURCE_DIR)/script/junit-okay.pl $($(NAME)_TEST_DIR)/report.xml; \
	if [ $$? -eq 0 ]; then \
		perl $(ROOT_SOURCE_DIR)/script/junit-report.pl --short $($(NAME)_TEST_DIR)/report.xml; \
	else \
		perl $(ROOT_SOURCE_DIR)/script/junit-report.pl $($(NAME)_TEST_DIR)/report.xml; \
	fi
	
.PHONY: $(NAME)-tests-all
$(NAME)-tests-all: override NAME := $(NAME)
$(NAME)-tests-all: $($(NAME)_BUILD_DIR)/$(NAME) $($(NAME)_BUILD_DIR)/test.mk | $($(NAME)_TEST_HOOKS)
	$(SAY) "SUITE" $@
	$(V)mkdir -p $($(NAME)_TEST_DIR)
	
ifeq ($(PARALLEL_TESTS),1)
	$(V)export OMP_NUM_THREADS=1; \
	$(MAKE) \
		-C $($(NAME)_TEST_DIR) \
		-f $(abspath $(ROOT_SOURCE_DIR))/$($(NAME)_BUILD_DIR)/test.mk \
		V='$(V)' SAY='$(SAY)' WITH_COVERAGE='$(WITH_COVERAGE)' LCOV='$(LCOV)'
        
	$(V)perl $(ROOT_SOURCE_DIR)/script/junit-combine.pl $($(NAME)_TEST_DIR)/report-*.xml > $(abspath $($(NAME)_TEST_DIR))/report.xml
	
ifeq ($(WITH_COVERAGE),1)
	$(SAY) 'LCOV' '$($(NAME)_COV_DIR)/coverage.info'
	$(V)merge_args=""; \
	for f in $($(NAME)_COV_DIR)/coverage-*.info; do \
	    merge_args="$$merge_args --add-tracefile $$f"; \
	done; \
	$(LCOV) $$merge_args \
		--output-file '$($(NAME)_COV_DIR)/coverage.info' \
		--ignore-errors inconsistent,inconsistent,unsupported,unsupported,format,format,empty,empty,count,count,unused,unused
	perl -pi -e 's|SF:$(abspath $(ROOT_SOURCE_DIR))/|SF:./|g' $($(NAME)_COV_DIR)/coverage.info
endif	

else
	$(V)mkdir -p $($(NAME)_TEST_DIR)
	$(V)cd $($(NAME)_TEST_DIR) && \
	GCOV_PREFIX_STRIP=999 GCOV_PREFIX='$(abspath $($(NAME)_COV_DIR))' $(abspath $($(NAME)_BUILD_DIR)/$(NAME)) \
		--reporter junit \
		--out $(abspath $($(NAME)_TEST_DIR))/report.xml

ifeq ($(WITH_COVERAGE),1)
	$(V)for f in $($(NAME)_BUILD_DIR)/*.gcno; do \
		ln -sf "$$(realpath $$f)" "$(abspath $($(NAME)_COV_DIR))/$$(basename $$f)"; \
	done
	$(SAY) 'LCOV' '$(abspath $($(NAME)_COV_DIR))/coverage.info'
	$(V)$(LCOV) --capture --directory "$(abspath $($(NAME)_COV_DIR))" --output-file "$(abspath $($(NAME)_COV_DIR))/coverage.info" --quiet \
		--ignore-errors inconsistent,inconsistent,unsupported,unsupported,format,format,empty,empty,count,count,unused,unused,version,version,gcov,gcov
	$(V)$(LCOV) --extract "$(abspath $($(NAME)_COV_DIR))/coverage.info" $(foreach d,$($(NAME)_COV_DIRS),$(abspath $(d))) --output-file "$(abspath $($(NAME)_COV_DIR))/coverage.info" --quiet \
		--ignore-errors inconsistent,inconsistent,unsupported,unsupported,format,format,empty,empty,count,count,unused,unused,version,version,gcov,gcov
endif

endif

TEST_TARGETS += $(NAME)-test	
