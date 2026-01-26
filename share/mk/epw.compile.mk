# Code for compilg C/C++ code and linking it.

# COMPILE_C_CMDS - Commands for compiling C source code.
define COMPILE_C_CMDS
	$(SAY) "CC" $$@
	$(V)mkdir -p $$(dir $$@)
	$(V)$$(strip $(CC) -o $$@ -c -MD $(CFLAGS) $($(NAME)_CFLAGS) $$($$<.CFLAGS) \
		-I$(ROOT_BUILD_DIR) -I$(ROOT_BUILD_DIR)/include \
		-I$($(NAME)_BUILD_DIR) -I$($(NAME)_BUILD_DIR)/include \
		$(addprefix -I,$($(NAME)_INCLUDE)) $$<)
	$$(PROCESS_DEPFILE)
endef

# COMPILE_CXX_CMDS - Commands for compiling C++ source code.
define COMPILE_CXX_CMDS
	$(SAY) "CXX" $$@
	$(V)mkdir -p $$(dir $$@)
	$(V)$$(strip $(CXX) -o $$@ -c -MD $(CXXFLAGS) $($(NAME)_CXXFLAGS) $$($$<.CXXFLAGS) \
		-I$(ROOT_BUILD_DIR) -I$(ROOT_BUILD_DIR)/include \
		-I$($(NAME)_BUILD_DIR) -I$($(NAME)_BUILD_DIR)/include \
		$(addprefix -I,$($(NAME)_INCLUDE)) $$<)
	$$(PROCESS_DEPFILE)
endef

# PROCESS_DEPFILE - Process a .d file into a .P file.
define PROCESS_DEPFILE
	$(V)cp $(@:%$(suffix $@)=%.d) $(@:%$(suffix $@)=%.P); \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	    -e '/^$$/ d' -e 's/$$/ :/' < $(@:%$(suffix $@)=%.d) \
	    >> $(@:%$(suffix $@)=%.P); \
	  rm -f $(@:%$(suffix $@)=%.d)
endef

# DEFINE_OBJ_RULE - Define a single object building rule.
define OBJ_RULE
$(1)/%.o: override NAME := $$(NAME)
$(1)/%.o: $(2)/%.$(3) $$($$(NAME)_PRECOMPILE_HOOKS)
	$(4)
endef