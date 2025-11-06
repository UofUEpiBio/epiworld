NQSAY := printf "  % -6s %s\n"
SAY := @$(NQSAY)

# QUALIFY_PATH - Given a "root" directory and one or more paths, qualifies the
#   paths using the "root" directory (i.e. appends the root directory name to
#   the paths) except for paths that are absolute.
define QUALIFY_PATH
$(addprefix ${1}/,$(filter-out /%,${2})) $(filter /%,${2})
endef

# CANONICAL_PATH - Given one or more paths, converts the paths to the canonical
#   form. The canonical form is the path, relative to the project's top-level
#   directory (the directory from which "make" is run), and without
#   any "./" or "../" sequences. For paths that are not  located below the
#   top-level directory, the canonical form is the absolute path (i.e. from
#   the root of the filesystem) also without "./" or "../" sequences.
define CANONICAL_PATH
$(patsubst ${CURDIR}/%,%,$(abspath ${1}))
endef

# PEEK - Parameterized "function" that results in the value at the top of the
#   specified colon-delimited stack.
#
# TODO: Use GMSL version.
define PEEK
$(lastword $(subst :, ,${1}))
endef

# POP - Parameterized "function" that pops the top value off of the specified
#   colon-delimited stack, and results in the new value of the stack. Note that
#   the popped value cannot be obtained using this function; use peek for that.
#
# TODO: Use GMSL version.
define POP
${1:%:$(lastword $(subst :, ,${1}))=%}
endef

# PUSH - Parameterized "function" that pushes a value onto the specified colon-
#   delimited stack, and results in the new value of the stack.
#
# TODO: Use GMSL version.
define PUSH
${2:%=${1}:%}
endef

# STACK_TO_PATH - Converts a stack into a path.
define STACK_TO_PATH
$(subst :,/,$(1))
endef

# CLEAN_CMDS - Emits commands for removing the specified file.
define CLEAN_CMDS
	$(SAY) "RM" ${1}
	$(V)rm -f ${1}
endef

# CLEAN_MULTIPLE_CMDS - Emits commands for removing the specified files.
define CLEAN_MULTIPLE_CMDS
 $(V)for file in $(1); do \
		$(NQSAY) RM $$file; \
		rm -f $$file; \
	done
endef

# EXEC_CMDS - Emits commands for exec'ing a binary.
define EXEC_CMDS
	$(SAY) "EXEC" "$(1) $(2)"
	$(V)if [ "x$(DIR)" = "x" ]; then \
		exec $(1) $(2); \
	else \
		cd $(DIR); exec $(1) $(2); \
	fi
endef