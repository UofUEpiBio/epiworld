ifeq ($($(NAME)_SOURCES),)
$(error $(NAME) has no sources)
endif

$(NAME)_OBJS             := \
	$(addprefix $($(NAME)_BUILD_DIR)/,$(addsuffix .o,$(basename $($(NAME)_SOURCES))))
#$(NAME)_OBJS             += \
#	$(subst $($(NAME)_PIA_DIR),$($(NAME)_OUT_DIR),$(addsuffix .o,$(basename $($(NAME)_GENSOURCES))))
$(NAME)_DEPFILES         := $($(NAME)_OBJS:%.o=%.P)
#$(NAME)_PRECOMPILE_HOOKS += \
#	$(addprefix $(ROOT_OUT_DIR)/sysroot/lib/lib,$(addsuffix .a,$($(NAME)_DEPLIBS)))

# C objects.
$(eval $(call OBJ_RULE,$($(NAME)_BUILD_DIR),$($(NAME)_SOURCE_DIR),c,$(COMPILE_C_CMDS)))
$(eval $(call OBJ_RULE,$($(NAME)_BUILD_DIR),$($(NAME)_BUILD_DIR),c,$(COMPILE_C_CMDS)))

# C++ objects.
$(eval $(call OBJ_RULE,$($(NAME)_BUILD_DIR),$($(NAME)_SOURCE_DIR),cpp,$(COMPILE_CXX_CMDS)))
$(eval $(call OBJ_RULE,$($(NAME)_BUILD_DIR),$($(NAME)_BUILD_DIR),cpp,$(COMPILE_CXX_CMDS)))
