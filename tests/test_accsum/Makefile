######################################
## Author: Eric Petit
## Mail eric.petit@prism.uvsq.fr
#####################################

#add -lpapi to ICC_L and GCC_L if it is installed on your machine

ICC=icc
ICC_OPT=-O3 -mp1
ICC_I= -I ./src -I /usr/include/ -I ./
ICC_L=
ICC_FLAG=

GCC=gcc
GCC_OPT= -O3 -march=core2 -fno-inline -funroll-all-loops -fopenmp
#-mstringop-strategy=loop only for complient gcc version
#-fopenmp option should appear only for open mp version
#unroll all loops interfer with hand unrolling
#If needed suppress this to option and reactivate line in "all" rule
GCC_I= -I ./src/ -I /usr/include/ -I ./
GCC_L= -fopenmp -lm -fno-inline
GCC_FLAG=-Wall -std=c99

LLVM=clang
LLVM_OPT=-O3
LLVM_I= -I ./src -I /usr/include/ -I ./
LLVM_L= -fopenmp -lm -fno-inline
LLVM_FLAG=

VERIFICARLO=verificarlo
VERIFICARLO_OPT=-O3
VERIFICARLO_I= -I ./src -I /usr/include/ -I ./
VERIFICARLO_L= -fopenmp -lm -fno-inline
VERIFICARLO_FLAG=




SRC_REP=./src
OBJ_REP_icc=./obj_icc
OBJ_REP_gcc=./obj_gcc
OBJ_REP_llvm=./obj_llvm
OBJ_REP_VERIFICARLO=./obj_verificarlo
BIN_REP=./bin

PRG_SRC_in=accSum.c accSumVect.c accSumPar.c DDsum.c dp_tools.c\
  FastAccSum.c FastAccSumb.c FastAccSumOpt_unroll4.c FastAccSumOpt_unroll3.c\
  FastAccSum_outlined_loops.c FastAccSumOpt_unroll2_outlined_loops.c\
  FastAccSumOpt_unroll2.c genSum_fromFile.c gensum.c main.c sum2.c

PRG_OBJ_in=$(patsubst %.c,%.o,$(PRG_SRC_in))
EXEC_NAME_in_icc=TestSum_icc
EXEC_NAME_in_gcc=TestSum_gcc
EXEC_NAME_in_llvm=TestSum_llvm
EXEC_NAME_in_VERIFICARLO=TestSum_verificarlo
GEN_SRC_in=dp_tools.c accSum.c gensum.c onlyGen.c
Gen_OBJ_in=$(patsubst %.c,%.o,$(GEN_SRC_in))
GEN_NAME_in=GenSum

PRG_SRC=$(patsubst %,$(SRC_REP)/%,$(PRG_SRC_in))
PRG_OBJ_icc=$(patsubst %,$(OBJ_REP_icc)/%,$(PRG_OBJ_in))
PRG_OBJ_gcc=$(patsubst %,$(OBJ_REP_gcc)/%,$(PRG_OBJ_in))
PRG_OBJ_llvm=$(patsubst %,$(OBJ_REP_llvm)/%,$(PRG_OBJ_in))
PRG_OBJ_VERIFICARLO=$(patsubst %,$(OBJ_REP_VERIFICARLO)/%,$(PRG_OBJ_in))


EXEC_NAME_icc=$(patsubst %,$(BIN_REP)/%,$(EXEC_NAME_in_icc))
EXEC_NAME_gcc=$(patsubst %,$(BIN_REP)/%,$(EXEC_NAME_in_gcc))
EXEC_NAME_llvm=$(patsubst %,$(BIN_REP)/%,$(EXEC_NAME_in_llvm))
EXEC_NAME_VERIFICARLO=$(patsubst %,$(BIN_REP)/%,$(EXEC_NAME_in_VERIFICARLO))

GEN_SRC=$(patsubst %,$(SRC_REP)/%,$(GEN_SRC_in))
GEN_OBJ=$(patsubst %,$(OBJ_REP)/%,$(GEN_OBJ_in))
GEN_NAME=$(patsubst %,$(BIN_REP)/%,$(GEN_NAME_in))

############################## all rules #############################

#rule to compil the bench
all: $(PRG_OBJ_icc) $(PRG_OBJ_gcc) $(PRG_OBJ_llvm) $(PRG_OBJ_VERIFICARLO)
	-mkdir -p $(BIN_REP)
	$(ICC)   $(ICC_L) -o $(EXEC_NAME_icc)  $(PRG_OBJ_icc)
	$(GCC)    $(GCC_L) -o $(EXEC_NAME_gcc)  $(PRG_OBJ_gcc) -lm
	$(LLVM)    $(LLVM_L) -o $(EXEC_NAME_llvm)  $(PRG_OBJ_llvm) -lm
	$(VERIFICARLO)    $(VERIFICARLO_L) -o $(EXEC_NAME_VERIFICARLO)  $(PRG_OBJ_VERIFICARLO) -lm 

#build only ICC version
all_icc: $(PRG_OBJ_icc)
	-mkdir -p $(BIN_REP)
	$(ICC)   $(ICC_L) -o $(EXEC_NAME_icc)  $(PRG_OBJ_icc)

#build only gcc version
all_gcc:  $(PRG_OBJ_gcc)
	-mkdir -p $(BIN_REP)
	$(GCC)   $(GCC_L) -o $(EXEC_NAME_gcc)  $(PRG_OBJ_gcc) -lm
#TODO: find a cleaner way to add -lm in the end, it does not work if -lm is at the begining in the linker flags...

#build only LLVM version
all_llvm: $(PRG_OBJ_llvm)
	-mkdir -p $(BIN_REP)
	$(LLVM)   $(LLVM_L) -o $(EXEC_NAME_llvm)  $(PRG_OBJ_llvm) -lm

#build only VERIFICARLO version
all_verificarlo: $(PRG_OBJ_VERIFICARLO)
	-mkdir -p $(BIN_REP)
	$(VERIFICARLO)   $(VERIFICARLO_L) -o $(EXEC_NAME_VERIFICARLO)  $(PRG_OBJ_VERIFICARLO) -lm




######################## input-data generator compilation rule #####################

#rule to compile the input data generator
all_gen: $(GEN_OBJ)
	$(CC)   $(CC_L) -o $(GEN_NAME)  $(GEN_OBJ)

######################### secondary rules for object generation #####################

#build icc object
$(OBJ_REP_icc)/%.o: $(SRC_REP)/%.c bench.conf
	-mkdir -p $(OBJ_REP_icc)
	$(ICC) $(ICC_OPT) $(ICC_FLAG) $(ICC_I)  -c $< -o $@
	#gen assembly file for checking generated code
	#$(ICC) $(ICC_OPT) $(ICC_FLAG) $(ICC_I) -S $< -o $@.s

#build gcc object
$(OBJ_REP_gcc)/%.o: $(SRC_REP)/%.c bench.conf
	-mkdir -p $(OBJ_REP_gcc)
	$(GCC) $(GCC_OPT) $(GCC_FLAG) $(GCC_I)  -c $< -o $@
	#gen assembly file for checking generated code
	#$(GCC) $(GCC_OPT) $(GCC_FLAG) $(GCC_I) -S $< -o $@.s

#build llvm object
$(OBJ_REP_llvm)/%.o: $(SRC_REP)/%.c bench.conf
	-mkdir -p $(OBJ_REP_llvm)
	$(LLVM) $(LLVM_OPT) $(LLVM_FLAG) $(LLVM_I)  -c $< -o $@
	#gen assembly file for checking generated code
	#$(LLVM) $(LLVM_OPT) $(LLVM_FLAG) $(LLVM_I) -S $< -o $@.s

#build llvm object
$(OBJ_REP_VERIFICARLO)/%.o: $(SRC_REP)/%.c bench.conf
	-mkdir -p $(OBJ_REP_VERIFICARLO)
	$(VERIFICARLO) $(VERIFICARLO_OPT) $(VERIFICARLO_FLAG) $(VERIFICARLO_I)  -c $< -o $@
	#gen assembly file for checking generated code
	#$(VERIFICARLO) $(VERIFICARLO_OPT) $(VERIFICARLO_FLAG) $(VERIFICARLO_I) -S $< -o $@.s


################## cleaning rule ################################
clean: clean_all

clean_all: clean_test clean_gen
	-rm -r $(BIN_REP)

clean_test:
	-rm -f $(PRG_OBJ_icc)
	-rm -f $(PRG_OBJ_gcc)
	-rm -f $(PRG_OBJ_llvm)
	-rm -f $(PRG_OBJ_VERIFICARLO)
	-rm -f $(EXEC_NAME_icc)
	-rm -f $(EXEC_NAME_gcc)
	-rm -f $(EXEC_NAME_llvm)
	-rm -f $(EXEC_NAME_VERIFICARLO)
	-rm -rf $(OBJ_REP_icc)
	-rm -rf $(OBJ_REP_gcc)
	-rm -rf $(OBJ_REP_llvm)
	-rm -rf $(OBJ_REP_VERIFICARLO)


clean_gen:
	-rm -f $(GEN_OBJ)
	-rm -f $(GEN_NAME)
