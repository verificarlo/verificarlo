ACLOCAL_AMFLAGS=-I m4
SUBDIRS=src/

dist_bin_SCRIPTS=verificarlo verificarlo-c verificarlo-f verificarlo-c++

edit_script = $(SED) -e 's,%LIBDIR%,$(libdir),'g $(NULL)

verificarlo: verificarlo.in Makefile
	$(AM_V_GEN)rm -f $@ $@.tmp && \
	$(edit_script) $< >$@.tmp && \
	chmod a-w $@.tmp && \
	mv $@.tmp $@

verificarlo-c:
	chmod a-w $@

verificarlo-c++:
	chmod a-w $@

verificarlo-f:
	chmod a-w $@

CLEANFILES = verificarlo

check:
	@echo "Tests should be run after install with make installcheck"

install-exec-hook:
	$(PYTHON) -m pip install . && \
	echo "Run 'source $(setenvdir)/set-env' to set environment variables"

installcheck:
# Run tests
# When successful print simple message
# If some tests have failed, report only the failing logs for the failed tests
	@if $(MAKE) -C tests/ check; then \
		echo "All tests have passed."; \
	else \
		grep FAIL tests/testplan.log | \
		while IFS= read -r line ; do \
			TEST=$$(echo $$line | cut -d' ' -f4); echo "************** TEST $${TEST}"; cat tests/$${TEST}/test.log; \
		done; \
		exit 1; \
	fi

# clean-local is a clean dependency of autotool clean target
clean-local: cleantests

cleantests:
# Clean tests directory
	cd tests/ && ./clean.sh && cd ..
	@echo "tests directory is clean."
