/********************************************************************************
 *                                                                              *
 *  This file is part of Verificarlo.                                           *
 *                                                                              *
 *  Copyright (c) 2015                                                          *
 *     Universite de Versailles St-Quentin-en-Yvelines                          *
 *     CMLA, Ecole Normale Superieure de Cachan                                 *
 *  Copyright (c) 2018                                                          *
 *     Universite de Versailles St-Quentin-en-Yvelines                          *
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
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <fstream>
#include <set>

#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR <= 6
#define CREATE_CALL2(func, op1, op2) (Builder.CreateCall2(func, op1, op2, ""))
#define CREATE_STRUCT_GEP(t, i, p) (Builder.CreateStructGEP(i, p))
#else
#define CREATE_CALL2(func, op1, op2) (Builder.CreateCall(func, {op1, op2}, ""))
#define CREATE_STRUCT_GEP(t, i, p) (Builder.CreateStructGEP(t, i, p, ""))
#endif

using namespace llvm;
// VfclibInst pass command line arguments
static cl::opt<std::string>
    VfclibInstFunction("vfclibinst-function",
                       cl::desc("Only instrument given FunctionName"),
                       cl::value_desc("FunctionName"), cl::init(""));

static cl::opt<std::string> VfclibInstFunctionFile(
    "vfclibinst-function-file",
    cl::desc("Instrument functions in file FunctionNameFile "),
    cl::value_desc("FunctionsNameFile"), cl::init(""));

static cl::opt<bool> VfclibInstVerbose("vfclibinst-verbose",
                                       cl::desc("Activate verbose mode"),
                                       cl::value_desc("Verbose"),
                                       cl::init(false));

namespace {
// Define an enum type to classify the floating points operations
// that are instrumented by verificarlo

enum Fops { FOP_ADD, FOP_SUB, FOP_MUL, FOP_DIV, FOP_CMP, FOP_IGNORE };

// Each instruction can be translated to a string representation

std::string Fops2str[] = {"add", "sub", "mul", "div", "cmp", "ignore"};

struct VfclibInst : public ModulePass {
  static char ID;

  std::set<std::string> SelectedFunctionSet;

  VfclibInst() : ModulePass(ID) {
    if (not VfclibInstFunctionFile.empty()) {
      std::string line;
      std::ifstream loopstream(VfclibInstFunctionFile.c_str());
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

  StructType *getMCAInterfaceType(IRBuilder<> &Builder) {

    // Verificarlo instrumentation calls the mca backend using
    // a vtable implemented as a structure.
    //
    // Here we declare the struct type corresponding to the
    // mca_interface_t defined in ../vfcwrapper/vfcwrapper.h
    //
    // Only the functions instrumented are declared. The last
    // three functions are user called functions and are not
    // needed here.

    SmallVector<Type *, 2> floatArgs, doubleArgs;
    floatArgs.push_back(Builder.getFloatTy());
    floatArgs.push_back(Builder.getFloatTy());
    doubleArgs.push_back(Builder.getDoubleTy());
    doubleArgs.push_back(Builder.getDoubleTy());

    PointerType *floatInstFun = PointerType::getUnqual(
        FunctionType::get(Builder.getFloatTy(), floatArgs, false));
    PointerType *doubleInstFun = PointerType::getUnqual(
        FunctionType::get(Builder.getDoubleTy(), doubleArgs, false));

    PointerType *floatcompInstFun = PointerType::getUnqual(
        FunctionType::get(Builder.getInt1Ty(), floatArgs, false));
    PointerType *doublecompInstFun = PointerType::getUnqual(
        FunctionType::get(Builder.getInt1Ty(), doubleArgs, false));

    return StructType::get(

        floatInstFun, floatInstFun, floatInstFun, floatInstFun,

        doubleInstFun, doubleInstFun, doubleInstFun, doubleInstFun,

        floatcompInstFun, floatcompInstFun, floatcompInstFun, floatcompInstFun,
        floatcompInstFun, floatcompInstFun, floatcompInstFun, floatcompInstFun,
        floatcompInstFun, floatcompInstFun, floatcompInstFun, floatcompInstFun,
        floatcompInstFun, floatcompInstFun, floatcompInstFun, floatcompInstFun,

        doublecompInstFun, doublecompInstFun, doublecompInstFun,
        doublecompInstFun, doublecompInstFun, doublecompInstFun,
        doublecompInstFun, doublecompInstFun, doublecompInstFun,
        doublecompInstFun, doublecompInstFun, doublecompInstFun,
        doublecompInstFun, doublecompInstFun, doublecompInstFun,
        doublecompInstFun,

        (void *)0);
  }

  bool runOnModule(Module &M) {
    bool modified = false;

    // Find the list of functions to instrument
    // Instrumentation adds stubs to mcalib function which we
    // never want to instrument.  Therefore it is important to
    // first find all the functions of interest before
    // starting instrumentation.

    std::vector<Function *> functions;
    for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
      const bool is_in =
          SelectedFunctionSet.find(F->getName()) != SelectedFunctionSet.end();
      if (SelectedFunctionSet.empty() || is_in) {
        functions.push_back(&*F);
      }
    }

    // Do the instrumentation on selected functions
    for (std::vector<Function *>::iterator F = functions.begin();
         F != functions.end(); ++F) {
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

  Instruction *replaceWithMCACall(Module &M, BasicBlock &B, Instruction *I,
                                  Fops opCode) {

    LLVMContext &Context = M.getContext();
    IRBuilder<> Builder(Context);
    StructType *mca_interface_type = getMCAInterfaceType(Builder);

    Type *retType = I->getType();
    Type *opType = I->getOperand(0)->getType();
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

    if (FCmpInst *FCI = dyn_cast<FCmpInst>(I)) {
      std::string mcaFunctionName = "_" + vectorName + baseTypeName + "_";
      // Comparison fct start à N°8
      int fct_position = 8;
      // Comparison with double starts at position +=16
      if (baseTypeName == "double")
        fct_position += 16;

      switch (FCI->getPredicate()) {
      case FCmpInst::FCMP_FALSE:
        fct_position += 0;
        mcaFunctionName += "false";
        break;
      case FCmpInst::FCMP_OEQ: // (A!=QNAN)&&(B!=QNAN)&&(A==B)
        fct_position += 1;
        mcaFunctionName += "oeq";
        break;
      case FCmpInst::FCMP_OGT: // (A!=QNAN)&&(B!=QNAN)&&(A>B)
        fct_position += 2;
        mcaFunctionName += "ogt";
        break;
      case FCmpInst::FCMP_OGE: // (A!=QNAN)&&(B!=QNAN)&&(A>=B)
        fct_position += 3;
        mcaFunctionName += "oge";
        break;
      case FCmpInst::FCMP_OLT: // (A!=QNAN)&&(B!=QNAN)&&(A<B)
        fct_position += 4;
        mcaFunctionName += "olt";
        break;
      case FCmpInst::FCMP_OLE: // (A!=QNAN)&&(B!=QNAN)&&(A<=B)
        fct_position += 5;
        mcaFunctionName += "ole";
        break;
      case FCmpInst::FCMP_ONE: // (A!=QNAN)&&(B!=QNAN)&&(A!=B)
        fct_position += 6;
        mcaFunctionName += "one";
        break;
      case FCmpInst::FCMP_ORD: // (A!=QNAN)&&(B!=QNAN)
        fct_position += 7;
        mcaFunctionName += "ord";
        break;
      case FCmpInst::FCMP_UEQ: // (A==QNAN)||(B==QNAN)||(A==B)
        fct_position += 8;
        mcaFunctionName += "ueq";
        break;
      case FCmpInst::FCMP_UGT: // (A!=QNAN)||(B!=QNAN)||(A>B)
        fct_position += 9;
        mcaFunctionName += "ugt";
        break;
      case FCmpInst::FCMP_UGE: // (A!=QNAN)||(B!=QNAN)||(A>=B)
        fct_position += 10;
        mcaFunctionName += "uge";
        break;
      case FCmpInst::FCMP_ULT: // (A!=QNAN)||(B!=QNAN)||(A<B)
        fct_position += 11;
        mcaFunctionName += "ult";
        break;
      case FCmpInst::FCMP_ULE: // (A!=QNAN)||(B!=QNAN)||(A<=B)
        fct_position += 12;
        mcaFunctionName += "ule";
        break;
      case FCmpInst::FCMP_UNE: // (A!=QNAN)||(B!=QNAN)||(A!=B)
        fct_position += 13;
        mcaFunctionName += "une";
        break;
      case FCmpInst::FCMP_UNO: // (A!=QNAN)||(B!=QNAN)
        fct_position += 14;
        mcaFunctionName += "uno";
        break;
      case FCmpInst::FCMP_TRUE:
        fct_position += 15;
        mcaFunctionName += "true";
        break;
      default:
        errs() << "Unsupported Comparison " << FCI->getPredicate() << "\n";
        assert(0);
      }

      // We use a builder adding instructions before the
      // instruction to replace
      Builder.SetInsertPoint(I);

      // Get a pointer to the global vtable
      // The vtable is accessed through the global structure
      // _vfc_current_mca_interface of type mca_interface_t which is
      // declared in ../vfcwrapper/vfcwrapper.c

      Constant *current_mca_interface =
          M.getOrInsertGlobal("_vfc_current_mca_interface", mca_interface_type);

      // Dereference the member at fct_position
      Value *arg_ptr = CREATE_STRUCT_GEP(mca_interface_type,
                                         current_mca_interface, fct_position);
      Value *fct_ptr = Builder.CreateLoad(arg_ptr, "");

      // Create a call instruction. It
      // will _replace_ I after it is returned.
      Instruction *newInst =
          CREATE_CALL2(fct_ptr, FCI->getOperand(0), FCI->getOperand(1));

      return newInst;

    }
    else if (vectorName != "") {
      // For vector types, helper functions in vfcwrapper are called
      std::string mcaFunctionName = "_" + vectorName + baseTypeName + opName;

      Constant *hookFunc = M.getOrInsertFunction(mcaFunctionName, retType,
                                                 opType, opType, (Type *)0);

      // For vector types we call directly a hardcoded helper function
      // no need to go through the vtable at this stage.
      Instruction *newInst =
          CREATE_CALL2(hookFunc, I->getOperand(0), I->getOperand(1));

      return newInst;
    }
    // For scalar types, we go directly through the struct of pointer function
    else {

      // We use a builder adding instructions before the
      // instruction to replace
      Builder.SetInsertPoint(I);

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
      Value *arg_ptr = CREATE_STRUCT_GEP(mca_interface_type,
                                         current_mca_interface, fct_position);
      Value *fct_ptr = Builder.CreateLoad(arg_ptr, "");

      // Create a call instruction. It
      // will _replace_ I after it is returned.
      Instruction *newInst =
          CREATE_CALL2(fct_ptr, I->getOperand(0), I->getOperand(1));

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
    case Instruction::FCmp:
      return FOP_CMP;
    default:
      return FOP_IGNORE;
    }
  }

  bool runOnBasicBlock(Module &M, BasicBlock &B) {

    bool modified = false;
    for (BasicBlock::iterator ii = B.begin(), ie = B.end(); ii != ie; ++ii) {
      Instruction &I = *ii;
      Fops opCode = mustReplace(I);
      if (opCode == FOP_IGNORE)
        continue;
      if (VfclibInstVerbose)
        errs() << "Instrumenting" << I << '\n';
      Instruction *newInst = replaceWithMCACall(M, B, &I, opCode);

      // Remove instruction from parent so it can be
      // inserted in a new context
      if (newInst->getParent() != NULL)
        newInst->removeFromParent();
      ReplaceInstWithInst(B.getInstList(), ii, newInst);
      modified = true;
    }

    return modified;
  }
};
} // namespace

char VfclibInst::ID = 0;
static RegisterPass<VfclibInst> X("vfclibinst", "verificarlo instrument pass", false, false);
