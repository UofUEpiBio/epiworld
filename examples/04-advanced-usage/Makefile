$(NAME)_SOURCES := \
	main.cpp

$(NAME)_README_HOOKS := \
	relocate-$(NAME)-resources	
	
include share/mk/epw.example.mk

.PHONY: relocate-$(NAME)-resources
relocate-$(NAME)-resources: override NAME := $(NAME)
relocate-$(NAME)-resources:
	$(SAY) "HOOK" $@
	$(V)cp -r $($(NAME)_SOURCE_DIR)/resources/* $($(NAME)_EXAMPLE_DIR)