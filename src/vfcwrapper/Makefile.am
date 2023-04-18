
include_HEADERS=vfcwrapper.c
vfcwrapper.c: main.c hashset.c
	@cat ../../config.h | grep 'LLVM_BINDIR' > vfcwrapper.c
	@echo "// vfcwrapper.c is automatically generated" >> vfcwrapper.c
	@echo "// do not modify this file directly" >> vfcwrapper.c
	@cat main.c funcinstr.c ../common/vfc_hashmap.c ../common/logger.c >> vfcwrapper.c

CLEANFILES=vfcwrapper.c
