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
GCC_OPT= -O3 -march=core2 -fno-inline -msse -funroll-all-loops -march=core2 -fopenmp
#-mstringop-strategy=loop only for complient gcc version
#-fopenmp option should appear only for open mp version
#unroll all loops interfer with hand unrolling
#If needed suppress this to option and reactivate line in "all" rule
GCC_I= -lm -I ./src/ -I /usr/include/ -I ./  
GCC_L= -fopenmp -lm -fno-inline 
GCC_FLAG=-Wall -std=c99 
 

SRC_REP=./src
OBJ_REP_icc=./obj_icc
OBJ_REP_gcc=./obj_gcc
BIN_REP=./bin

PRG_SRC_in=accSum.c accSumVect.c accSumPar.c DDsum.c dp_tools.c\
  FastAccSum.c FastAccSumb.c FastAccSumOpt_unroll4.c FastAccSumOpt_unroll3.c\
  FastAccSum_outlined_loops.c FastAccSumOpt_unroll2_outlined_loops.c\
  FastAccSumOpt_unroll2.c genSum_fromFile.c gensum.c main.c sum2.c 

PRG_OBJ_in=$(patsubst %.c,%.o,$(PRG_SRC_in))
EXEC_NAME_in_icc=TestSum_icc
EXEC_NAME_in_gcc=TestSum_gcc
GEN_SRC_in=dp_tools.c accSum.c gensum.c onlyGen.c
Gen_OBJ_in=$(patsubst %.c,%.o,$(GEN_SRC_in))
GEN_NAME_in=GenSum

PRG_SRC=$(patsubst %,$(SRC_REP)/%,$(PRG_SRC_in))
PRG_OBJ_icc=$(patsubst %,$(OBJ_REP_icc)/%,$(PRG_OBJ_in))
PRG_OBJ_gcc=$(patsubst %,$(OBJ_REP_gcc)/%,$(PRG_OBJ_in))

EXEC_NAME_icc=$(patsubst %,$(BIN_REP)/%,$(EXEC_NAME_in_icc))
EXEC_NAME_gcc=$(patsubst %,$(BIN_REP)/%,$(EXEC_NAME_in_gcc))


GEN_SRC=$(patsubst %,$(SRC_REP)/%,$(GEN_SRC_in))
GEN_OBJ=$(patsubst %,$(OBJ_REP)/%,$(GEN_OBJ_in))
GEN_NAME=$(patsubst %,$(BIN_REP)/%,$(GEN_NAME_in))

############################## all rules #############################

#rule to compil the bench
all: $(PRG_OBJ_icc) $(PRG_OBJ_gcc)
	$(ICC)   $(ICC_L) -o $(EXEC_NAME_icc)  $(PRG_OBJ_icc)
	$(GCC)   $(GCC_L) -o $(EXEC_NAME_gcc)  $(PRG_OBJ_gcc)
	
#build only ICC version
all_icc: $(PRG_OBJ_icc)
	$(ICC)   $(ICC_L) -o $(EXEC_NAME_icc)  $(PRG_OBJ_icc)

#build only gcc version
all_gcc:  $(PRG_OBJ_gcc)
	$(GCC)   $(GCC_L) -o $(EXEC_NAME_gcc)  $(PRG_OBJ_gcc) -lm
#TODO: find a cleaner way to add -lm in the end, it does not work if -lm is at the begining in the linker flags... 	
######################## input-data generator compilation rule #####################

#rule to compile the input data generator	
-all_gen: $(GEN_OBJ)
	$(CC)   $(CC_L) -o $(GEN_NAME)  $(GEN_OBJ)
	
######################### secondary rules for object generation #####################	

#build icc object
$(OBJ_REP_icc)/%.o: $(SRC_REP)/%.c
	$(ICC) $(ICC_OPT) $(ICC_FLAG) $(ICC_I)  -c $< -o $@
	$(ICC) $(ICC_OPT) $(ICC_FLAG) $(ICC_I) -S $< -o $@.s

#build gcc object
$(OBJ_REP_gcc)/%.o: $(SRC_REP)/%.c 
	$(GCC) $(GCC_OPT) $(GCC_FLAG) $(GCC_I)  -c $< -o $@
	#$(GCC) $(GCC_OPT) $(GCC_FLAG) $(GCC_I) -S $< -o $@.s


################## cleaning rule ################################
##--TODO-- in the proper way...
clean:
	-@rm $(PRG_OBJ_icc)
	-@rm $(PRG_OBJ_gcc)
	-@rm $(EXEC_NAME)

clean_gen:
	-@rm $(GEN_OBJ)
	-@rm $(GEN_NAME)
