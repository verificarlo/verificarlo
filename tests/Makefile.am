TEST_LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) \
                  $(top_srcdir)/autoconf/tap-driver.sh
TESTS = testplan.test
EXTRA_DIST = $(TESTS)

# Generate clean list
generateclean:
	@if $(shell git --version) ; then                         \
		git clean -d -n | cut -d' ' -f2 > clean_list.txt; \
	fi

# Clean tests folder
cleantests:
	@cat clean_list.txt |        \
	while IFS= read -r line ; do \
		rm -Rf $$line;       \
	done
