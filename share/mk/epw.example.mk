include share/mk/epw.prog.mk

$(NAME)_EXAMPLE_DIR := $($(NAME)_BUILD_DIR)/.example
EXAMPLE_TARGETS += $($(NAME)_BUILD_DIR)/$(NAME)

.PHONY: mk-$(NAME)-working-dir
mk-$(NAME)-working-dir: override NAME := $(NAME)
mk-$(NAME)-working-dir:
	$(SAY) "HOOK" $@
	$(V)mkdir -p $($(NAME)_EXAMPLE_DIR)

$($(NAME)_SOURCE_DIR)/README.md: override NAME := $(NAME)
$($(NAME)_SOURCE_DIR)/README.md: $($(NAME)_BUILD_DIR)/$(NAME) | mk-$(NAME)-working-dir $($(NAME)_README_HOOKS)
	$(SAY) "GEN" $@
	$(V)cd $($(NAME)_EXAMPLE_DIR)
	$(V)printf "## Example: \`$(NAME)\`\n" > $@
	$(V)printf "\n" >> $@
	$(V)printf "Output from the program:\n\n" >> $@
	$(V)printf "\`\`\`\n" >> $@
	$(V)cd $($(NAME)_EXAMPLE_DIR) && $(abspath $(ROOT_SOURCE_DIR))/$($(NAME)_BUILD_DIR)/$(NAME) \
	    >> $(abspath $(ROOT_SOURCE_DIR))/$@
	$(V)printf "\`\`\`\n" >> $@

README_TARGETS += $($(NAME)_SOURCE_DIR)/README.md