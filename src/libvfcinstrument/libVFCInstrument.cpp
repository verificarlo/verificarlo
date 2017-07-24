/********************************************************************************
 *                                                                              *
 *  This file is part of Verificarlo.                                           *
 *                                                                              *
 *  Copyright (c) 2015                                                          *
 *     Universite de Versailles St-Quentin-en-Yvelines                          *
 *     CMLA, Ecole Normale Superieure de Cachan                                 *
 *                                                                              *
 *  Verificarlo is free software: you can redistribute it and/or modify         *
 *  it under the terms of the GNU General Public License as published by        *
 *  the Free Software Foundation, either version 3 of the License, or           *
 *  (at your option) any later version.                                         *
 *                                                                              *
 *  Verificarlo is distributed in the hope that it will be useful,              *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
 *  GNU General Public License for more details.                                *
 *                                                                              *
 *  You should have received a copy of the GNU General Public License           *
 *  along with Verificarlo.  If not, see <http://www.gnu.org/licenses/>.        *
 *                                                                              *
 ********************************************************************************/

#include "../../config.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/TypeBuilder.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <set>
#include <fstream>

#if LLVM_VERSION_MINOR <= 6
#define CREATE_CALL2(func, op1, op2) (builder.CreateCall2(func, op1, op2, ""))
#define CREATE_STRUCT_GEP(i, p) (builder.CreateStructGEP(i, p))
#else
#define CREATE_CALL2(func, op1, op2) (builder.CreateCall(func, {op1, op2}, ""))
#define CREATE_STRUCT_GEP(i, p) (builder.CreateStructGEP(nullptr, i, p, ""))
#endif

using namespace llvm;
// VfclibInst pass command line arguments
static cl::opt<std::string> VfclibInstFunction("vfclibinst-function",
					       cl::desc("Only instrument given FunctionName"),
					       cl::value_desc("FunctionName"), cl::init(""));

static cl::opt<std::string> VfclibInstFunctionFile("vfclibinst-function-file",
						   cl::desc("Instrument functions in file FunctionNameFile "),
						   cl::value_desc("FunctionsNameFile"), cl::init(""));

static cl::opt<bool> VfclibInstVerbose("vfclibinst-verbose",
				       cl::desc("Activate verbose mode"),
				       cl::value_desc("Verbose"), cl::init(false));


static cl::opt<bool> VfclibBlackList("vfclibblack-list",
				     cl::desc("Activate black list mode"),
				     cl::value_desc("BlackList"), cl::init(false));



namespace {
    // Define an enum type to classify the floating points operations
    // that are instrumented by verificarlo

    enum Fops {FOP_ADD, FOP_SUB, FOP_MUL, FOP_DIV, FOP_IGNORE};

    // Each instruction can be translated to a string representation

    std::string Fops2str[] = { "add", "sub", "mul", "div", "ignore"};

    struct VfclibInst : public ModulePass {
        static char ID;
        StructType * mca_interface_type;
        std::set<std::string> SelectedFunctionSet;
        std::set<std::string> BlackListFunctionSet;

        VfclibInst() : ModulePass(ID) {
	  if (not VfclibInstFunctionFile.empty()) {
	    std::string line;
	    std::ifstream loopstream (VfclibInstFunctionFile.c_str());
	    if (loopstream.is_open()) {
	      while (std::getline(loopstream, line)) {
		SelectedFunctionSet.insert(line);
	      }
	      loopstream.close();
	    } else {
	      errs() << "Cannot open " << VfclibInstFunctionFile << "\n";
	      assert(0);
	    }
	  } else if (not VfclibInstFunction.empty()) {
	    SelectedFunctionSet.insert(VfclibInstFunction);
	  }
	}

        StructType * getMCAInterfaceType() {
            LLVMContext &Context =getGlobalContext();

            // Verificarlo instrumentation calls the mca backend using
            // a vtable implemented as a structure.
            //
            // Here we declare the struct type corresponding to the
            // mca_interface_t defined in ../vfcwrapper/vfcwrapper.h
            //
            // Only the functions instrumented are declared. The last
            // three functions are user called functions and are not
            // needed here.

            return StructType::get(

                TypeBuilder<float(*)(float, float), false>::get(Context),
                TypeBuilder<float(*)(float, float), false>::get(Context),
                TypeBuilder<float(*)(float, float), false>::get(Context),
                TypeBuilder<float(*)(float, float), false>::get(Context),


                TypeBuilder<double(*)(double, double), false>::get(Context),
                TypeBuilder<double(*)(double, double), false>::get(Context),
                TypeBuilder<double(*)(double, double), false>::get(Context),
                TypeBuilder<double(*)(double, double), false>::get(Context),

                (void *)0
                );
        }


        bool runOnModule(Module &M) {
            bool modified = false;

            mca_interface_type = getMCAInterfaceType();

            // Find the list of functions to instrument
            // Instrumentation adds stubs to mcalib function which we
            // never want to instrument.  Therefore it is important to
            // first find all the functions of interest before
            // starting instrumentation.

            std::vector<Function*> functions;
            for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
	      const bool is_in = SelectedFunctionSet.find(F->getName()) != SelectedFunctionSet.end();
	      if (SelectedFunctionSet.empty() || VfclibBlackList != is_in) {
		functions.push_back(&*F);
	      }
	    }

            // Do the instrumentation on selected functions
            for(std::vector<Function*>::iterator F = functions.begin(); F != functions.end(); ++F) {
                modified |= runOnFunction(M, **F);
            }
	    
	    // runOnModule must return true if the pass modifies the IR
            return modified;
        }

        bool runOnFunction(Module &M, Function &F) {
            if (VfclibInstVerbose) {
                errs() << "In Function: ";
                errs().write_escaped(F.getName()) << '\n';
            }

            bool modified = false;

            for (Function::iterator bi = F.begin(), be = F.end(); bi != be; ++bi) {
                modified |= runOnBasicBlock(M, *bi);
            }
            return modified;
        }

        Instruction *replaceWithMCACall(Module &M, BasicBlock &B, Instruction * I, Fops opCode) {
            Type * retType = I->getType();
            Type * opType = I->getOperand(0)->getType();
            std::string opName = Fops2str[opCode];

            std::string baseTypeName = "";
            std::string vectorName = "";
            Type *baseType = opType;

            // Check for vector types
            if (opType->isVectorTy()) {
                VectorType *t = static_cast<VectorType *>(opType);
                baseType = t->getElementType();
                unsigned size = t->getNumElements();

                if (size == 2) {
                    vectorName = "2x";
                } else if (size == 4) {
                    vectorName = "4x";
                } else {
                    errs() << "Unsuported vector size: " << size << "\n";
                    assert(0);
                }
            }

            // Check the type of the operation
            if (baseType->isDoubleTy()) {
                baseTypeName = "double";
            } else if (baseType->isFloatTy()) {
                baseTypeName = "float";
            } else {
                errs() << "Unsupported operand type: " << *opType << "\n";
                assert(0);
            }

            // For vector types, helper functions in vfcwrapper are called
            if (vectorName != "") {
                std::string mcaFunctionName = "_" + vectorName + baseTypeName + opName;

                Constant *hookFunc = M.getOrInsertFunction(mcaFunctionName,
                                                           retType,
                                                           opType,
                                                           opType,
                                                           (Type *) 0);

                // For vector types we call directly a hardcoded helper function
                // no need to go through the vtable at this stage.
                IRBuilder<> builder(getGlobalContext());
                Instruction *newInst = CREATE_CALL2(hookFunc,
                                                    I->getOperand(0), I->getOperand(1));

                return newInst;
            }
            // For scalar types, we go directly through the struct of pointer function
            else {

                // We use a builder adding instructions before the
                // instruction to replace
                IRBuilder<> builder(I);

                // Get a pointer to the global vtable
                // The vtable is accessed through the global structure
                // _vfc_current_mca_interface of type mca_interface_t which is
                // declared in ../vfcwrapper/vfcwrapper.c

                Constant *current_mca_interface =
                    M.getOrInsertGlobal("_vfc_current_mca_interface", mca_interface_type);

                // Compute the position of the required member fct pointer
                // opCodes are ordered in the same order than the struct members :-)
                // There are 4 float members followed by 4 double members.
                int fct_position = opCode;
                if (baseTypeName == "double") fct_position += 4;

                // Dereference the member at fct_position
                Value *arg_ptr = CREATE_STRUCT_GEP(current_mca_interface, fct_position);
                Value *fct_ptr = builder.CreateLoad(arg_ptr, false);

                // Create a call instruction. It is important to
                // create the instruction in the globalcontext, indeed
                // the instruction is not to be inserted before I. It
                // will _replace_ I after it is returned.
                IRBuilder<> builder2(getGlobalContext());

                Instruction *newInst = CREATE_CALL2(
                    fct_ptr,
                    I->getOperand(0), I->getOperand(1));

                return newInst;
            }
        }


        Fops mustReplace(Instruction &I) {
            switch (I.getOpcode()) {
                case Instruction::FAdd:
                    return FOP_ADD;
                case Instruction::FSub:
                    // In LLVM IR the FSub instruction is used to represent FNeg
                    return FOP_SUB;
                case Instruction::FMul:
                    return FOP_MUL;
                case Instruction::FDiv:
                    return FOP_DIV;
                default:
                    return FOP_IGNORE;
            }
        }

        bool runOnBasicBlock(Module &M, BasicBlock &B) {
            bool modified = false;
            for (BasicBlock::iterator ii = B.begin(), ie = B.end(); ii != ie; ++ii) {
                Instruction &I = *ii;
                Fops opCode = mustReplace(I);
                if (opCode == FOP_IGNORE) continue;
                if (VfclibInstVerbose) errs() << "Instrumenting" << I << '\n';
                Instruction *newInst = replaceWithMCACall(M, B, &I, opCode);
                // Remove instruction from parent so it can be
                // inserted in a new context
                if (newInst->getParent() != NULL) newInst->removeFromParent();
                ReplaceInstWithInst(B.getInstList(), ii, newInst);
                modified = true;
            }
            return modified;
        }
    };
}

char VfclibInst::ID = 0;
static RegisterPass<VfclibInst> X("vfclibinst", "verificarlo instrument pass", false, false);

