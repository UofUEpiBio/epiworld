# Test case: %HUMAN_NAME%
ifeq ($(WITH_COVERAGE),1)
%COV_DIR%/coverage-%RULE_NAME%.info: COV_SILO = %COV_DIR%/coverage-%RULE_NAME%
%COV_DIR%/coverage-%RULE_NAME%.info: %TEST_DIR%/report-%RULE_NAME%.xml
	$(SAY) 'LCOV' '$(COV_SILO)/coverage.info'
	$(V)for f in %BUILD_DIR%/*.gcno; do \
        ln -sf "$$(realpath $$f)" "$(COV_SILO)/$$(basename $$f)"; \
    done
	$(V)$(LCOV) --capture --directory "$(COV_SILO)" --output-file "$(COV_SILO)/coverage.info.all" --quiet \
	    --ignore-errors inconsistent,inconsistent,unsupported,unsupported,format,format,empty,empty,count,count,unused,unused,version,version,gcov,gcov
	$(V)$(LCOV) --extract "$(COV_SILO)/coverage.info.all" "%COV_DIRS%" --output-file "%COV_DIR%/coverage-%RULE_NAME%.info" --quiet \
	    --ignore-errors inconsistent,inconsistent,unsupported,unsupported,format,format,empty,empty,count,count,unused,unused,version,version,gcov,gcov
endif

%TEST_DIR%/report-%RULE_NAME%.xml: TTYP = %TEST_DIR%/.tty-%RULE_NAME%
%TEST_DIR%/report-%RULE_NAME%.xml: COV_SILO = %COV_DIR%/coverage-%RULE_NAME%
%TEST_DIR%/report-%RULE_NAME%.xml: %BINARY%
	$(SAY) 'TEST' '%HUMAN_NAME%'
	$(V)mkdir -p $(COV_SILO)
	$(V)GCOV_PREFIX_STRIP=999 GCOV_PREFIX='$(COV_SILO)' %BINARY% \
		--reporter junit \
		--out %TEST_DIR%/report-%RULE_NAME%.xml \
		'%HUMAN_NAME%' \
		>$(TTYP) 2>&1; \
	perl -pi -e 's/name="tests"/name="%HUMAN_NAME%"/g' %TEST_DIR%/report-%RULE_NAME%.xml; \
	cat '$(TTYP)'; \
	rm '$(TTYP)'

.PHONY: %RULE_NAME%
ifeq ($(WITH_COVERAGE),1)
%RULE_NAME%: %COV_DIR%/coverage-%RULE_NAME%.info
else
%RULE_NAME%: %TEST_DIR%/report-%RULE_NAME%.xml
endif