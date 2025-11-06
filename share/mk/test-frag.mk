# Test case: %HUMAN_NAME%
.PHONY: %RULE_NAME%
%RULE_NAME%: SILO = %COV_DIR%/coverage-%RULE_NAME%
%RULE_NAME%: TTYP = %COV_DIR%/coverage-%RULE_NAME%/.tty
%RULE_NAME%:
	$(SAY) 'TEST' '%HUMAN_NAME%'
	$(V)mkdir -p $(SILO)
	$(V)GCOV_PREFIX_STRIP=999 GCOV_PREFIX='$(SILO)' %BINARY% -n '%HUMAN_NAME%' --use-colour yes >$(TTYP) 2>&1
	$(V)cat '$(TTYP)'
	$(V)rm '$(TTYP)'
	
ifneq ($(WITH_COVERAGE),)
	$(SAY) 'LCOV' '$(SILO)/coverage.info'
	$(V)for f in %BUILD_DIR%/*.gcno; do \
        ln -sf "$$(realpath $$f)" "$(SILO)/$$(basename $$f)"; \
    done
	$(V)$(LCOV) --capture --directory "$(SILO)" --output-file "$(SILO)/coverage.info.all" --quiet \
	    --ignore-errors inconsistent,inconsistent,unsupported,unsupported,format,format,empty,empty,count,count,unused,unused
	$(V)$(LCOV) --extract "$(SILO)/coverage.info.all" "%SOURCE_DIR%" --output-file "%COV_DIR%/coverage-%RULE_NAME%.info" --quiet \
	    --ignore-errors inconsistent,inconsistent,unsupported,unsupported,format,format,empty,empty,count,count,unused,unused
endif