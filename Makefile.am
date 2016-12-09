ACLOCAL_AMFLAGS=-I m4
SUBDIRS=src/ tests/
dist_bin_SCRIPTS=verificarlo


edit_script = $(SED) -e 's,%LIBDIR%,$(libdir),'g $(NULL)

verificarlo: verificarlo.in Makefile
	$(AM_V_GEN)rm -f $@ $@.tmp && \
	$(edit_script) $< >$@.tmp && \
	chmod a-w $@.tmp && \
	mv $@.tmp $@

CLEANFILES = verificarlo
