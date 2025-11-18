include share/mk/epw.prog.mk

$(NAME)_TEST_DIR := $($(NAME)_BUILD_DIR)/.test
$(NAME)_COV_DIR := $($(NAME)_BUILD_DIR)/.coverage
	
# This looks complicated, but all it's really doing is:
# 
#   1. Run the test suite with the -l flag to list all test cases.
#   2. For each test case, generate a Makefile rule that runs that test case.
#   3. Create a target "all" that depends on all the test case rules.
#   4. The generated Makefile is stored in the build directory as test.mk.
# 
# This means that we can run tests in parallel using `make -j` and each test case
# will be run in its own rule. This also allows us to easily add new test cases without
# modifying the Makefile, as the test cases are discovered at build time.
$($(NAME)_BUILD_DIR)/test.mk: override NAME := $(NAME)
$($(NAME)_BUILD_DIR)/test.mk: $($(NAME)_BUILD_DIR)/$(NAME) $($(NAME)_TEST_HOOKS)
	$(SAY) "GEN" $@
	$(V)printf "# Auto-generated test Makefile for $(NAME)\n" > $@
	$(V)printf "\n" >> $@
	$(V)printf ".SUFFIXES:\n" >> $@
	$(V)printf ".DEFAULT_GOAL := all\n" >> $@
	$(V)printf "MAKEFLAGS     += --no-builtin-rules --no-builtin-variables\n" >> $@
	$(V)printf "\n" >> $@
	$(V)all_tests=$$( ( $($(NAME)_BUILD_DIR)/$(NAME) -l || true ) | \
	    perl -nE 'if (/^ {2}(\S.*)$$/) { $$s = $$1; $$s =~ s/^\s+|\s+$$//g; say $$s }'); \
	printf '%s\n' "$$all_tests" > $@.tmp; \
    test_targets=""; \
    while IFS= read -r test; do \
        rule_name=$$(printf '%s' "$$test" | sha256sum | cut -d' ' -f1); \
        test_targets="$$test_targets $$rule_name"; \
        cat share/mk/test-frag.mk | \
            sed "s/%RULE_NAME%/$$rule_name/g" | \
            sed "s/%HUMAN_NAME%/$$test/g" | \
            sed "s|%BINARY%|$(abspath $($(NAME)_BUILD_DIR))/$(NAME)|g" | \
            sed "s|%BUILD_DIR%|$(abspath $($(NAME)_BUILD_DIR))|g" | \
            sed "s|%SOURCE_DIR%|$(abspath $($(NAME)_SOURCE_DIR))|g" | \
            sed "s|%COV_DIR%|$(abspath $($(NAME)_COV_DIR))|g" >> $@; \
        printf "\n\n" >> $@; \
    done < $@.tmp; \
    rm -f $@.tmp; \
    printf 'all:%s\n' "$$test_targets" >> $@

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
.PHONY: $($(NAME)_SOURCE_DIR)-test
$($(NAME)_SOURCE_DIR)-test: override NAME := $(NAME)
$($(NAME)_SOURCE_DIR)-test: $($(NAME)_BUILD_DIR)/test.mk
	$(SAY) "SUITE" $@
	$(V)mkdir -p $($(NAME)_TEST_DIR)
	$(V)$(MAKE) \
        -C $($(NAME)_TEST_DIR) \
        -f $(abspath $(ROOT_SOURCE_DIR))/$($(NAME)_BUILD_DIR)/test.mk \
        V='$(V)' SAY='$(SAY)' WITH_COVERAGE='$(WITH_COVERAGE)' LCOV='$(LCOV)'

ifneq ($(WITH_COVERAGE),)
	$(SAY) 'LCOV' '$($(NAME)_COV_DIR)/en-total.info'
	$(V)args=""; \
	for f in $($(NAME)_COV_DIR)/coverage-*.info; do \
	    args="$$args --add-tracefile $$f"; \
	done; \
	exec $(LCOV) $$args \
	    --output-file '$($(NAME)_COV_DIR)/coverage.info' \
		--ignore-errors inconsistent,inconsistent,unsupported,unsupported,format,format,empty,empty,count,count,unused,unused
	sed -i '' 's|SF:$(abspath $(ROOT_SOURCE_DIR))/|SF:./|g' '$($(NAME)_COV_DIR)/coverage.info'
endif

TEST_TARGETS += $($(NAME)_SOURCE_DIR)-test	

#$(V)cd $($(NAME)_TEST_DIR) && $(abspath $(ROOT_SOURCE_DIR))/$($(NAME)_BUILD_DIR)/$(NAME) \
	#--warn NoAssertions --order rand --shard-index 0 --shard-count 3 --rng-seed 0xBEEF
