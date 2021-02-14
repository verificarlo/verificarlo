/******************************************************************************
 *                                                                            *
 *  This file is part of Verificarlo.                                         *
 *                                                                            *
 *  Copyright (c) 2020                                                        *
 *     Verificarlo contributors                                               *
 *                                                                            *
 *  Verificarlo is free software: you can redistribute it and/or modify       *
 *  it under the terms of the GNU General Public License as published by      *
 *  the Free Software Foundation, either version 3 of the License, or         *
 *  (at your option) any later version.                                       *
 *                                                                            *
 *  Verificarlo is distributed in the hope that it will be useful,            *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 *  GNU General Public License for more details.                              *
 *                                                                            *
 *  You should have received a copy of the GNU General Public License         *
 *  along with Verificarlo.  If not, see <http://www.gnu.org/licenses/>.      *
 *                                                                            *
 ******************************************************************************/

#include "../../config.h"
#include "llvm/Analysis/CallGraphSCCPass.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Mangler.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/UnifyFunctionExitNodes.h"

#include <cxxabi.h>
#include <fstream>
#include <iostream>
#include <set>
#include <stdio.h>
#include <string>
#include <utility>
#include <vector>

using namespace llvm;

namespace {

static Function *func_enter;
static Function *func_exit;

// Enumeration of managed types
enum Ftypes { FLOAT, DOUBLE, FLOAT_PTR, DOUBLE_PTR };

// Types
llvm::Type *FloatTy, *DoubleTy, *FloatPtrTy, *DoublePtrTy, *Int8Ty, *Int8PtrTy,
    *Int32Ty;

// Array of values
Value *Types2val[] = {NULL, NULL, NULL, NULL};

// Fill use_double and use_float with true if the call_inst pi use at least
// of the managed types
void haveFloatingPointArithmetic(Instruction *call, Function *f,
                                 bool is_from_library, bool is_intrinsic,
                                 bool *use_float, bool *use_double, Module &M) {
  Type *ReturnTy;

  if (f) {
    ReturnTy = f->getReturnType();
  } else {
    ReturnTy = call->getType();
  }

  // Test if return type of call is float
  (*use_float) = ReturnTy == FloatTy;

  // Test if return type of call is double
  (*use_double) = ReturnTy == DoubleTy;

  // Test if f treat floats point numbers
  if (f != NULL && f->size() != 0) {
    // Loop over each instruction of the function and test if one of them
    // use float or double
    for (auto &bbi : (*f)) {
      for (auto &ii : bbi) {
        for (size_t i = 0; i < ii.getNumOperands(); i++) {
          Type *opType = ii.getOperand(i)->getType();

          if (opType->isVectorTy()) {
            VectorType *t = static_cast<VectorType *>(opType);
            opType = t->getElementType();
          }

          if (opType == FloatTy)
            (*use_float) = true;

          if (opType == DoubleTy)
            (*use_double) = true;
        }
      }
    }
  } else if (call != NULL) {
    // Loop over arguments types
    for (auto it = call->op_begin(); it < call->op_end() - 1; it++) {
      if ((*it)->getType() == FloatTy || (*it)->getType() == FloatPtrTy)
        (*use_float) = true;
      if ((*it)->getType() == DoubleTy || (*it)->getType() == DoublePtrTy)
        (*use_double) = true;
    }
  }
}

// Search the size of the Value V which is a pointer
unsigned int getSizeOf(Value *V, const Function *F) {
  // if V is an argument of the F function, search the size of V in the parent
  // of F
  for (auto &Args : F->args()) {
    if (&Args == V) {
      for (const auto &U : F->users()) {
        if (const CallInst *call = cast<CallInst>(U)) {
          Value *to_search = call->getOperand(Args.getArgNo());

          return getSizeOf(to_search, call->getParent()->getParent());
        }
      }
    }
  }

  // search for the AllocaInst at the origin of V
  for (auto &BB : (*F)) {
    for (auto &I : BB) {
      if (&I == V) {
        if (const AllocaInst *Alloca = dyn_cast<AllocaInst>(&I)) {
          if (Alloca->getAllocatedType()->isArrayTy() ||
              Alloca->getAllocatedType()->isVectorTy()) {
            return Alloca->getAllocatedType()->getArrayNumElements();
          } else {
            return 1;
          }
        } else if (const GetElementPtrInst *GEP =
                       dyn_cast<GetElementPtrInst>(&I)) {
          Value *to_search = GEP->getOperand(0);
          return getSizeOf(to_search, F);
        }
      }
    }
  }

  return 0;
}

// Get the Name of the given argument V
std::string getArgName(Function *F, Value *V, unsigned int i) {
  for (auto &BB : (*F)) {
    for (auto &I : BB) {
      if (isa<CallInst>(&I)) {
        CallInst *Call = cast<CallInst>(&I);

        if (Call->getCalledFunction()->getName() == "llvm.dbg.declare" ||
            Call->getCalledFunction()->getName() == "llvm.dbg.value" ||
            Call->getCalledFunction()->getName() == "llvm.dbg.addr") {
          DILocalVariable *Var = cast<DILocalVariable>(
              cast<MetadataAsValue>(I.getOperand(1))->getMetadata());

          if (Var->isParameter() && (Var->getArg() == i + 1)) {
            return Var->getName().str();
          }
        }
      }
    }
  }

  return "parameter_" + std::to_string(i + 1);
}

void InstrumentFunction(std::vector<Value *> MetaData,
                        Function *CurrentFunction, Function *HookedFunction,
                        const CallInst *call, BasicBlock *B, Module &M) {
  IRBuilder<> Builder(B);

  // Step 1: add space on the heap for pointers
  size_t output_cpt = 0;
  std::vector<Value *> OutputAlloca;

  if (HookedFunction->getReturnType() == DoubleTy) {
    OutputAlloca.push_back(Builder.CreateAlloca(DoubleTy, nullptr));
    output_cpt++;
  } else if (HookedFunction->getReturnType() == FloatTy) {
    OutputAlloca.push_back(Builder.CreateAlloca(FloatTy, nullptr));
    output_cpt++;
  } else if (HookedFunction->getReturnType() == FloatPtrTy && call) {
    output_cpt++;
  } else if (HookedFunction->getReturnType() ==
                 Type::getDoublePtrTy(M.getContext()) &&
             call) {
    output_cpt++;
  }

  size_t input_cpt = 0;
  std::vector<Value *> InputAlloca;

  for (auto &args : CurrentFunction->args()) {
    if (args.getType() == DoubleTy) {
      InputAlloca.push_back(Builder.CreateAlloca(DoubleTy, nullptr));
      input_cpt++;
    } else if (args.getType() == FloatTy) {
      InputAlloca.push_back(Builder.CreateAlloca(FloatTy, nullptr));
      input_cpt++;
    } else if (args.getType() == FloatPtrTy && call) {
      input_cpt++;
      output_cpt++;
    } else if (args.getType() == Type::getDoublePtrTy(M.getContext()) && call) {
      input_cpt++;
      output_cpt++;
    }
  }

  std::vector<Value *> InputMetaData = MetaData;
  InputMetaData.push_back(ConstantInt::get(Builder.getInt32Ty(), input_cpt));

  std::vector<Value *> OutputMetaData = MetaData;
  OutputMetaData.push_back(ConstantInt::get(Builder.getInt32Ty(), output_cpt));

  // Step 2: for each function input (arguments), add its type, size, name and
  // address to the list of parameters sent to vfc_enter for processing.
  std::vector<Value *> EnterArgs = InputMetaData;
  size_t input_index = 0;
  for (auto &args : CurrentFunction->args()) {
    if (args.getType() == DoubleTy) {
      EnterArgs.push_back(Types2val[DOUBLE]);
      EnterArgs.push_back(Builder.CreateGlobalStringPtr(
          getArgName(HookedFunction, &args, args.getArgNo())));
      EnterArgs.push_back(ConstantInt::get(Int32Ty, 1));
      EnterArgs.push_back(InputAlloca[input_index]);
      Builder.CreateStore(&args, InputAlloca[input_index++]);
    } else if (args.getType() == FloatTy) {
      EnterArgs.push_back(Types2val[FLOAT]);
      EnterArgs.push_back(Builder.CreateGlobalStringPtr(
          getArgName(HookedFunction, &args, args.getArgNo())));
      EnterArgs.push_back(ConstantInt::get(Int32Ty, 1));
      EnterArgs.push_back(InputAlloca[input_index]);
      Builder.CreateStore(&args, InputAlloca[input_index++]);
    } else if (args.getType() == FloatPtrTy && call) {
      EnterArgs.push_back(Types2val[FLOAT_PTR]);
      EnterArgs.push_back(Builder.CreateGlobalStringPtr(
          getArgName(HookedFunction, &args, args.getArgNo())));
      EnterArgs.push_back(
          ConstantInt::get(Int32Ty, getSizeOf(call->getOperand(args.getArgNo()),
                                              call->getParent()->getParent())));
      EnterArgs.push_back(&args);
    } else if (args.getType() == DoublePtrTy && call) {
      EnterArgs.push_back(Types2val[DOUBLE_PTR]);
      EnterArgs.push_back(Builder.CreateGlobalStringPtr(
          getArgName(HookedFunction, &args, args.getArgNo())));
      EnterArgs.push_back(
          ConstantInt::get(Int32Ty, getSizeOf(call->getOperand(args.getArgNo()),
                                              call->getParent()->getParent())));
      EnterArgs.push_back(&args);
    }
  }

  // Step 3: call vfc_enter
  Builder.CreateCall(func_enter, EnterArgs);

  // Step 4: load modified values
  std::vector<Value *> FunctionArgs;
  input_index = 0;
  for (auto &args : CurrentFunction->args()) {
    if (args.getType() == DoubleTy) {
      FunctionArgs.push_back(
          Builder.CreateLoad(DoubleTy, InputAlloca[input_index++]));
    } else if (args.getType() == FloatTy) {
      FunctionArgs.push_back(
          Builder.CreateLoad(FloatTy, InputAlloca[input_index++]));
    } else {
      FunctionArgs.push_back(&args);
    }
  }

  // Step 5: call hooked function with modified values
  Value *ret;
  if (call) {
    CallInst *hook = cast<CallInst>(call->clone());
    int i = 0;
    for (auto &args : FunctionArgs)
      hook->setArgOperand(i++, args);
    hook->setCalledFunction(HookedFunction);
    ret = Builder.Insert(hook);
  } else {
    CallInst *call = CallInst::Create(HookedFunction, FunctionArgs);
    call->setAttributes(HookedFunction->getAttributes());
    call->setCallingConv(HookedFunction->getCallingConv());
    ret = Builder.Insert(call);
  }

  // Step 6: for each function output (return value, and pointers as argument),
  // add its type, size, name and address to the list of parameters sent to
  // vfc_exit for processing.
  std::vector<Value *> ExitArgs = OutputMetaData;
  if (ret->getType() == DoubleTy) {
    ExitArgs.push_back(Types2val[DOUBLE]);
    ExitArgs.push_back(Builder.CreateGlobalStringPtr("return_value"));
    ExitArgs.push_back(ConstantInt::get(Int32Ty, 1));
    ExitArgs.push_back(OutputAlloca[0]);
    Builder.CreateStore(ret, OutputAlloca[0]);
  } else if (ret->getType() == FloatTy) {
    ExitArgs.push_back(Types2val[FLOAT]);
    ExitArgs.push_back(Builder.CreateGlobalStringPtr("return_value"));
    ExitArgs.push_back(ConstantInt::get(Int32Ty, 1));
    ExitArgs.push_back(OutputAlloca[0]);
    Builder.CreateStore(ret, OutputAlloca[0]);
  } else if (HookedFunction->getReturnType() == FloatPtrTy && call) {
    ExitArgs.push_back(Types2val[FLOAT_PTR]);
    ExitArgs.push_back(Builder.CreateGlobalStringPtr("return_value"));
    ExitArgs.push_back(ConstantInt::get(
        Int32Ty, getSizeOf(ret, call->getParent()->getParent())));
    ExitArgs.push_back(ret);
  } else if (HookedFunction->getReturnType() == DoublePtrTy && call) {
    ExitArgs.push_back(Types2val[DOUBLE_PTR]);
    ExitArgs.push_back(Builder.CreateGlobalStringPtr("return_value"));
    ExitArgs.push_back(ConstantInt::get(
        Int32Ty, getSizeOf(ret, call->getParent()->getParent())));
    ExitArgs.push_back(ret);
  }

  for (auto &args : CurrentFunction->args()) {
    if (args.getType() == FloatPtrTy && call) {
      ExitArgs.push_back(Types2val[FLOAT_PTR]);
      ExitArgs.push_back(Builder.CreateGlobalStringPtr(
          getArgName(HookedFunction, &args, args.getArgNo())));
      ExitArgs.push_back(
          ConstantInt::get(Int32Ty, getSizeOf(call->getOperand(args.getArgNo()),
                                              call->getParent()->getParent())));
      ExitArgs.push_back(&args);
    } else if (args.getType() == DoublePtrTy && call) {
      ExitArgs.push_back(Types2val[DOUBLE_PTR]);
      ExitArgs.push_back(Builder.CreateGlobalStringPtr(
          getArgName(HookedFunction, &args, args.getArgNo())));
      ExitArgs.push_back(
          ConstantInt::get(Type::getInt32Ty(M.getContext()),
                           getSizeOf(call->getOperand(args.getArgNo()),
                                     call->getParent()->getParent())));
      ExitArgs.push_back(&args);
    }
  }

  // Step 7: call vfc_exit
  Builder.CreateCall(func_exit, ExitArgs);

  // Step 8: load the modified return value
  if (ret->getType() == DoubleTy) {
    ret = Builder.CreateLoad(DoubleTy, OutputAlloca[0]);
  } else if (ret->getType() == FloatTy) {
    ret = Builder.CreateLoad(FloatTy, OutputAlloca[0]);
  }

  // Step 9: return the modified return value if necessary
  if (HookedFunction->getReturnType() != Builder.getVoidTy()) {
    Builder.CreateRet(ret);
  } else {
    Builder.CreateRetVoid();
  }
}

struct VfclibFunc : public ModulePass {
  static char ID;
  std::vector<Function *> OriginalFunctions;
  std::vector<Function *> ClonedFunctions;
  size_t inst_cpt;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<TargetLibraryInfoWrapperPass>();
  }

  VfclibFunc() : ModulePass(ID) { inst_cpt = 1; }

  virtual bool runOnModule(Module &M) {
    TargetLibraryInfoWrapperPass TLIWP;

    FloatTy = Type::getFloatTy(M.getContext());
    FloatPtrTy = Type::getFloatPtrTy(M.getContext());
    DoubleTy = Type::getDoubleTy(M.getContext());
    DoublePtrTy = Type::getDoublePtrTy(M.getContext());
    Int8Ty = Type::getInt8Ty(M.getContext());
    Int8PtrTy = Type::getInt8PtrTy(M.getContext());
    Int32Ty = Type::getInt32Ty(M.getContext());

    Types2val[0] = ConstantInt::get(Int32Ty, 0);
    Types2val[1] = ConstantInt::get(Int32Ty, 1);
    Types2val[2] = ConstantInt::get(Int32Ty, 2);
    Types2val[3] = ConstantInt::get(Int32Ty, 3);

    /*************************************************************************
     *                  Get original functions's names                       *
     *************************************************************************/
    for (auto &F : M) {
      if ((F.getName().str() != "main") && F.size() != 0) {
        OriginalFunctions.push_back(&F);
      }
    }

    /*************************************************************************
     *                  Enter and exit functions declarations                *
     *************************************************************************/

    std::vector<Type *> ArgTypes{Int8PtrTy, Int8Ty, Int8Ty,
                                 Int8Ty,    Int8Ty, Int32Ty};

    // Signature of enter_function and exit_function
    FunctionType *FunTy =
        FunctionType::get(Type::getVoidTy(M.getContext()), ArgTypes, true);

    // void vfc_enter_function (char*, char, char, char, char, int, ...)
    func_enter = Function::Create(FunTy, Function::ExternalLinkage,
                                  "vfc_enter_function", &M);
    func_enter->setCallingConv(CallingConv::C);

    // void vfc_exit_function (char*, char, char, char, char, int, ...)
    func_exit = Function::Create(FunTy, Function::ExternalLinkage,
                                 "vfc_exit_function", &M);
    func_exit->setCallingConv(CallingConv::C);

    /*************************************************************************
     *                             Main special case                         *
     *************************************************************************/
    if (M.getFunction("main")) {
      Function *Main = M.getFunction("main");

      ValueToValueMapTy VMap;
      Function *Clone = CloneFunction(Main, VMap);

      DISubprogram *Sub = Main->getSubprogram();
      std::string Name = Sub->getName().str();
      std::string File = Sub->getFilename().str();
      std::string Line = std::to_string(Sub->getLine());
      std::string NewName = "vfc_" + File + "//" + Name + "/" + Line + "/" +
                            std::to_string(inst_cpt) + "_hook";
      std::string FunctionName =
          File + "//" + Name + "/" + Line + "/" + std::to_string(++inst_cpt);

      bool use_float, use_double;

      // Test if the function use double or float
      haveFloatingPointArithmetic(NULL, Main, 0, 0, &use_float, &use_double, M);

      // Delete Main Body
      Main->deleteBody();

      BasicBlock *block = BasicBlock::Create(M.getContext(), "block", Main);
      IRBuilder<> Builder(block);

      // Create function ID
      Value *FunctionID = Builder.CreateGlobalStringPtr(FunctionName);

      // Constants creation
      Constant *isLibraryFunction = ConstantInt::get(Int8Ty, 0);
      Constant *isInstrinsicFunction = ConstantInt::get(Int8Ty, 0);
      Constant *haveFloat = ConstantInt::get(Int8Ty, use_float);
      Constant *haveDouble = ConstantInt::get(Int8Ty, use_double);

      // Enter metadata arguments
      std::vector<Value *> MetaData{FunctionID, isLibraryFunction,
                                    isInstrinsicFunction, haveFloat,
                                    haveDouble};

      Clone->setName(NewName);

      InstrumentFunction(MetaData, Main, Clone, NULL, block, M);

      OriginalFunctions.push_back(Clone);
    }

    /*************************************************************************
     *                             Function calls                            *
     *************************************************************************/
    for (auto &F : OriginalFunctions) {
      if (F->getSubprogram()) {
        std::string Parent = F->getSubprogram()->getName().str();
        for (auto &B : (*F)) {
          IRBuilder<> Builder(&B);

          for (auto ii = B.begin(); ii != B.end();) {
            Instruction *pi = &(*ii++);

            if (isa<CallInst>(pi)) {
              // collect metadata info //
              if (Function *f = cast<CallInst>(pi)->getCalledFunction()) {

                if (MDNode *N = pi->getMetadata("dbg")) {
                  DILocation *Loc = cast<DILocation>(N);
                  DISubprogram *Sub = f->getSubprogram();
                  unsigned line = Loc->getLine();
                  std::string File = Loc->getFilename().str();
                  std::string Name;

                  if (Sub) {
                    Name = Sub->getName().str();
                  } else {
                    Name = f->getName().str();
                  }

                  std::string Line = std::to_string(line);
                  std::string NewName = "vfc_" + File + "/" + Parent + "/" +
                                        Name + "/" + Line + "/" +
                                        std::to_string(inst_cpt) + +"_hook";

                  std::string FunctionName = File + "/" + Parent + "/" + Name +
                                             "/" + Line + "/" +
                                             std::to_string(++inst_cpt);

                  // Test if f is a library function //
#if LLVM_VERSION_MAJOR >= 10
                  const TargetLibraryInfo &TLI = TLIWP.getTLI(*f);
#else
                  const TargetLibraryInfo &TLI = TLIWP.getTLI();
#endif

                  LibFunc libfunc;

                  bool is_from_library = TLI.getLibFunc(f->getName(), libfunc);

                  // Test if f is instrinsic //
                  bool is_intrinsic = f->isIntrinsic();

                  // Test if the function use double or float
                  bool use_float, use_double;
                  haveFloatingPointArithmetic(pi, f, is_from_library,
                                              is_intrinsic, &use_float,
                                              &use_double, M);

                  // If the called function is an intrinsic function that does
                  // not use float or double, do not instrument it.
                  if (is_intrinsic && !(use_float || use_double)) {
                    continue;
                  }

                  // Create function ID
                  Value *FunctionID =
                      Builder.CreateGlobalStringPtr(FunctionName);

                  // Constants creation
                  Constant *isLibraryFunction =
                      ConstantInt::get(Int8Ty, is_from_library);
                  Constant *isInstrinsicFunction =
                      ConstantInt::get(Int8Ty, is_intrinsic);
                  Constant *haveFloat = ConstantInt::get(Int8Ty, use_float);
                  Constant *haveDouble = ConstantInt::get(Int8Ty, use_double);

                  // Enter function arguments
                  std::vector<Value *> MetaData{FunctionID, isLibraryFunction,
                                                isInstrinsicFunction, haveFloat,
                                                haveDouble};

                  Type *ReturnTy = f->getReturnType();
                  std::vector<Type *> CallTypes;
                  for (auto it = pi->op_begin(); it < pi->op_end() - 1; it++) {
                    CallTypes.push_back(cast<Value>(it)->getType());
                  }

                  // Create the hook function
                  FunctionType *HookFunTy =
                      FunctionType::get(ReturnTy, CallTypes, false);
                  Function *hook_func = Function::Create(
                      HookFunTy, Function::ExternalLinkage, NewName, &M);

                  // Gives to the hook function the calling convention and
                  // attributes of the original function.
                  hook_func->setAttributes(f->getAttributes());
                  hook_func->setCallingConv(f->getCallingConv());

                  BasicBlock *block =
                      BasicBlock::Create(M.getContext(), "block", hook_func);

                  // Instrument the original function call
                  InstrumentFunction(MetaData, hook_func, f, cast<CallInst>(pi),
                                     block, M);
                  // Replace the call to the original function by a call to the
                  // hook function
                  cast<CallInst>(pi)->setCalledFunction(hook_func);
                }
              }
            }
          }
        }
      }
    }

    return true;
  }
}; // namespace

} // namespace

char VfclibFunc::ID = 0;
static RegisterPass<VfclibFunc>
    X("vfclibfunc", "verificarlo function instrumentation pass", false, false);
