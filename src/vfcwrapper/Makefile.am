
include_HEADERS=vfcwrapper.c
vfcwrapper.c: main.c hashset.c
	@echo "// vfcwrapper.c is automatically generated" > vfcwrapper.c
	@echo "// do not modify this file directly" >> vfcwrapper.c
	@cat main.c funcinstr.c >> vfcwrapper.c

CLEANFILES=vfcwrapper.c
