include share/mk/epw.prog.mk

$(NAME)_EXAMPLE_DIR := $($(NAME)_BUILD_DIR)/.example
EXAMPLE_TARGETS += $($(NAME)_BUILD_DIR)/$(NAME)
EXAMPLE_RUN_TARGETS += example-$(NAME)-run

.PHONY: mk-$(NAME)-working-dir
mk-$(NAME)-working-dir: override NAME := $(NAME)
mk-$(NAME)-working-dir:
	$(SAY) "HOOK" $@
	$(V)mkdir -p $($(NAME)_EXAMPLE_DIR)

.PHONY: example-$(NAME)-run
example-$(NAME)-run: override NAME := $(NAME)
example-$(NAME)-run: $($(NAME)_BUILD_DIR)/$(NAME) | mk-$(NAME)-working-dir
	$(SAY) "RUN" $@
	$(V)cd $($(NAME)_EXAMPLE_DIR) && $(abspath $(ROOT_SOURCE_DIR))/$($(NAME)_BUILD_DIR)/$(NAME)
