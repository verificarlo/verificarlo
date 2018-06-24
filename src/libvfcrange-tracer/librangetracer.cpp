/********************************************************************************
 *                                                                              *
 *  This file is part of Verificarlo.                                           *
 *                                                                              *
 *  Copyright (c) 2017                                                          *
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
#include <string>
#include <sstream>
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/TypeBuilder.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/ADT/Twine.h"

#include <set>
#include <fstream>

#if LLVM_VERSION_MINOR == 5
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/InstIterator.h"
#else
#include "llvm/DebugInfo.h"
#include "llvm/Support/InstIterator.h"
#endif

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
                                       cl::value_desc("Verbose"),
                                       cl::init(false));

static cl::opt<bool> VfclibBlackList("vfclibblack-list",
				     cl::desc("Activate black list mode"),
				     cl::value_desc("BlackList"), cl::init(false));

namespace {

struct VfclibRangeTracer : public ModulePass {
  static char ID;
  StructType *mca_interface_type;
  std::set<std::string> SelectedFunctionSet;

  VfclibRangeTracer() : ModulePass(ID) {
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

  const Function *findEnclosingFunc(const Value *V) {
    if (const Argument *Arg = dyn_cast<Argument>(V)) {
      return Arg->getParent();
    }
    if (const Instruction *I = dyn_cast<Instruction>(V)) {
      return I->getParent()->getParent();
    }
    return NULL;
  }

  const MDNode *findVar(const Value *V, const Function *F) {
    for (const_inst_iterator Iter = inst_begin(F), End = inst_end(F);
         Iter != End; ++Iter) {
      const Instruction *I = &*Iter;
      if (const DbgValueInst *DbgValue = dyn_cast<DbgValueInst>(I)) {
        if (DbgValue->getValue() == V)
          return DbgValue->getVariable();
      } else if (const DbgDeclareInst *DbgDeclare =
                     dyn_cast<DbgDeclareInst>(I)) {
        if (DbgDeclare->getAddress() == V)
          return DbgDeclare->getVariable();
      }
    }
    return NULL;
  }

  StringRef getOriginalName(const Value *V) {

    // If the value is defined as a GetElementPtrInstruction, return the name
    // of the pointer operand instead
    if (const GetElementPtrInst *I = dyn_cast<GetElementPtrInst>(V)) {
      return getOriginalName(I->getPointerOperand());
    }
    // If the value is a constant Expr, such as a GetElementPtrInstConstantExpr,
    // try to get the name of its first operand
    if (const ConstantExpr *E = dyn_cast<ConstantExpr>(V)) {
      return getOriginalName(E->getOperand(0));
    }

    const Function *F = findEnclosingFunc(V);
    if (!F)
      return V->getName();

    const MDNode *Var = findVar(V, F);
    if (!Var)
      return "_";

    return DIVariable(Var).getName();
  }

  // Use std::string instead of StringRef
  // Sources: http://llvm.org/docs/ProgrammersManual.html#dss-twine
  //
  // "2.For the same reason, StringRef cannot be used as the return value
  //   of a method if the method “computes” the result string. Instead, use std::string."
  std::string getOriginalLine(const Instruction &I) {
    std::string str_to_return = "_ _";  
    if (MDNode *N = I.getMetadata(LLVMContext::MD_dbg)) {
      DILocation Loc(N);
      std::string Line = std::to_string(Loc.getLineNumber());
      std::string File = Loc.getFilename();
      std::string Dir = Loc.getDirectory();
      str_to_return = File + " " + Line;
    } 
    return str_to_return;
  }

  bool runOnModule(Module &M) {
    bool modified = false;

    StringRef SelectedFunction = StringRef(VfclibInstFunction);

    // Find the list of functions to instrument
    // Instrumentation adds stubs to mcalib function which we
    // never want to instrument.  Therefore it is important to
    // first find all the functions of interest before
    // starting instrumentation.

    std::vector<Function *> functions;
    for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
      const bool is_in = SelectedFunctionSet.find(F->getName()) != SelectedFunctionSet.end();
      if (SelectedFunctionSet.empty() || VfclibBlackList != is_in) {
        functions.push_back(&*F);
      } 
    }

    // Do the instrumentation on selected functions
    for (std::vector<Function *>::iterator F = functions.begin(); F != functions.end(); ++F) {
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
      modified |= runOnBasicBlock(M, *bi, F.getName());
    }
    
    return modified;
  }

  bool runOnBasicBlock(Module &M, BasicBlock &B, StringRef functionName) {
    bool modified = false;
    for (BasicBlock::iterator ii = B.begin(), ie = B.end(); ii != ie; ++ii) {
      Instruction &I = *ii;
      if (I.getOpcode() == Instruction::Store) {

	StringRef variableName = getOriginalName((I.getOperand(1)));
	std::string variableLine = getOriginalLine(I);

	errs() << "Instrumenting" << I
	       << " Variable Name = " << variableName
	       << " at " << variableLine << '\n';
	
        std::string baseTypeName = "";
        Type *baseType = I.getOperand(0)->getType();
        Type *ptrType = I.getOperand(1)->getType();

        // XXX deal with vector types
        assert(!baseType->isVectorTy());

        Type *voidTy = Type::getVoidTy(M.getContext());
        Type *charPtrTy = Type::getInt8PtrTy(M.getContext());

        if (baseType->isDoubleTy()) {
          baseTypeName = "binary64";
        } else if (baseType->isFloatTy()) {
          baseTypeName = "binary32";
        } else if (baseType->isIntegerTy()) {
	    baseTypeName = "int";
	} else {
          // Ignore non floating types
          continue;
        }

        std::string mcaFunctionName = "_verificarlo_output_" + baseTypeName;

        Constant *hookFunc = M.getOrInsertFunction(
            mcaFunctionName, voidTy, baseType, ptrType, charPtrTy, (Type *)0);
        IRBuilder<> builder(ii);

        std::string locationInfo = variableLine + " " +
	                           functionName.str() + " " +
	                           variableName.str() + " " +
	                           baseTypeName;

        Value *strPtr = builder.CreateGlobalStringPtr(locationInfo, ".str");
        Instruction *newInst = builder.CreateCall3(cast<Function>(hookFunc),
                                                   I.getOperand(0), I.getOperand(1), strPtr, "");
        modified = true;
      }
    }
    return modified;
  }
};
}

char VfclibRangeTracer::ID = 0;
static RegisterPass<VfclibRangeTracer> X("vfclibrange-tracer", "verificarlo range tracer pass",
                                  false, false);
