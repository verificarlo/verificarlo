
include_HEADERS=vfcwrapper.c
vfcwrapper.c: main.c hashset.c
	@cat ../../config.h | grep 'ADDR2LINE_PATH' > vfcwrapper.c
	@echo "// vfcwrapper.c is automatically generated" >> vfcwrapper.c
	@echo "// do not modify this file directly" >> vfcwrapper.c
	@cat main.c funcinstr.c >> vfcwrapper.c

CLEANFILES=vfcwrapper.c
