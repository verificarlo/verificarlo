/*****************************************************************************\
 *                                                                           *\
 *  This file is part of the Verificarlo project,                            *\
 *  under the Apache License v2.0 with LLVM Exceptions.                      *\
 *  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception.                 *\
 *  See https://llvm.org/LICENSE.txt for license information.                *\
 *                                                                           *\
 *                                                                           *\
 *  Copyright (c) 2015                                                       *\
 *     Universite de Versailles St-Quentin-en-Yvelines                       *\
 *     CMLA, Ecole Normale Superieure de Cachan                              *\
 *                                                                           *\
 *  Copyright (c) 2018                                                       *\
 *     Universite de Versailles St-Quentin-en-Yvelines                       *\
 *                                                                           *\
 *  Copyright (c) 2019-2026                                                  *\
 *     Verificarlo Contributors                                              *\
 *                                                                           *\
 ****************************************************************************/
#include "../../config.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Mangler.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#ifdef PIC
#undef PIC
#endif
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Casting.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"
#pragma GCC diagnostic pop
#include <stdio.h>
#include <string>

typedef llvm::LibFunc _LibFunc;

#if LLVM_VERSION_MAJOR >= 18
#define STARTS_WITH(str, prefix) str.starts_with(prefix)
#else
#define STARTS_WITH(str, prefix) str.startswith(prefix)
#endif

using namespace llvm;

namespace {

static Function *func_enter;
static Function *func_exit;

/* from interflop.h*/
/* Enumeration of types managed by function instrumentation */
enum FTYPES {
  FFLOAT,
  FDOUBLE,
  FQUAD,
  FFLOAT_PTR,
  FDOUBLE_PTR,
  FQUAD_PTR,
  FTYPES_END
};

// Types
llvm::Type *FloatTy, *DoubleTy, *FloatPtrTy, *DoublePtrTy, *Int8Ty, *Int8PtrTy,
    *Int32Ty;

// Array of values
Value *Types2val[] = {
    [FFLOAT] = NULL,     [FDOUBLE] = NULL,     [FQUAD] = NULL,
    [FFLOAT_PTR] = NULL, [FDOUBLE_PTR] = NULL, [FQUAD_PTR] = NULL,
    [FTYPES_END] = NULL};

// Fill use_double and use_float with true if the call_inst pi use at least
// of the managed types
void haveFloatingPointArithmetic(Instruction *call, Function *f,
                                 bool *use_float, bool *use_double) {
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
      for (const auto &U : V->users()) {
        if (isa<CallInst>(U)) {
          const CallInst *call = cast<CallInst>(U);
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
std::string getArgName(Function *F, unsigned int i) {
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

void allocateMemoryForPointers(Function *CurrentFunction,
                               Function *HookedFunction, IRBuilder<> &Builder,
                               std::vector<Value *> &InputAlloca,
                               std::vector<Value *> &OutputAlloca,
                               size_t &input_cpt, size_t &output_cpt,
                               const CallInst *call) {

  Type *RetTy = HookedFunction->getReturnType();
  if (RetTy == DoubleTy or RetTy == FloatTy) {
    OutputAlloca.push_back(Builder.CreateAlloca(RetTy, nullptr));
    output_cpt++;
  } else if ((RetTy == FloatPtrTy or RetTy == DoublePtrTy) and call) {
    output_cpt++;
  }

  for (auto &args : CurrentFunction->args()) {
    Type *argTy = args.getType();
    if (argTy == DoubleTy or argTy == FloatTy) {
      InputAlloca.push_back(Builder.CreateAlloca(argTy, nullptr));
      input_cpt++;
    } else if ((argTy == FloatPtrTy or argTy == DoublePtrTy) and call) {
      input_cpt++;
      output_cpt++;
    }
  }
};

FTYPES ftypesFromType(Type *Ty) {
  if (Ty == FloatTy)
    return FFLOAT;
  if (Ty == DoubleTy)
    return FDOUBLE;
  if (Ty == FloatPtrTy)
    return FFLOAT_PTR;
  if (Ty == DoublePtrTy)
    return FDOUBLE_PTR;
  return FTYPES_END;
}

void initializeInputArgs(std::vector<Value *> &EnterArgs,
                         Function *CurrentFunction, Function *HookedFunction,
                         const CallInst *call, IRBuilder<> &Builder,
                         std::vector<Value *> &InputAlloca) {
  size_t input_index = 0;
  for (auto &args : CurrentFunction->args()) {
    Type *argTy = args.getType();
    FTYPES type = ftypesFromType(argTy);

    if (type != FTYPES_END) {
      std::string arg_name = getArgName(HookedFunction, args.getArgNo());
      EnterArgs.push_back(Types2val[type]);
      EnterArgs.push_back(Builder.CreateGlobalStringPtr(arg_name));
    }

    if (argTy == DoubleTy or argTy == FloatTy) {
      EnterArgs.push_back(ConstantInt::get(Int32Ty, 1));
      EnterArgs.push_back(InputAlloca[input_index]);
      Builder.CreateStore(&args, InputAlloca[input_index++]);
    } else if ((argTy == FloatPtrTy or argTy == DoublePtrTy) and call) {
      unsigned int size = getSizeOf(call->getOperand(args.getArgNo()),
                                    call->getParent()->getParent());
      EnterArgs.push_back(ConstantInt::get(Int32Ty, size));
      EnterArgs.push_back(&args);
    }
  }
}

void initializeOutputArgs(std::vector<Value *> &ExitArgs,
                          Function *CurrentFunction, Function *HookedFunction,
                          Value *ret, const CallInst *call,
                          IRBuilder<> &Builder,
                          std::vector<Value *> &OutputAlloca) {

  Type *retTy = ret->getType();
  FTYPES type = ftypesFromType(retTy);

  if (type != FTYPES_END) {
    ExitArgs.push_back(Types2val[type]);
    ExitArgs.push_back(Builder.CreateGlobalStringPtr("return_value"));
  }

  if (retTy == FloatTy or retTy == DoubleTy) {
    ExitArgs.push_back(ConstantInt::get(Int32Ty, 1));
    ExitArgs.push_back(OutputAlloca[0]);
    Builder.CreateStore(ret, OutputAlloca[0]);
  } else if ((retTy == FloatPtrTy or retTy == DoublePtrTy) and
             call != nullptr) {
    unsigned int size = getSizeOf(ret, call->getParent()->getParent());
    ExitArgs.push_back(ConstantInt::get(Int32Ty, size));
    ExitArgs.push_back(ret);
  }

  for (auto &args : CurrentFunction->args()) {
    Type *argTy = args.getType();
    type = ftypesFromType(argTy);
    if ((argTy == FloatPtrTy or argTy == DoublePtrTy) and call != nullptr) {
      std::string arg_name = getArgName(HookedFunction, args.getArgNo());
      ExitArgs.push_back(Types2val[type]);
      ExitArgs.push_back(Builder.CreateGlobalStringPtr(arg_name));
      unsigned int size = getSizeOf(call->getOperand(args.getArgNo()),
                                    call->getParent()->getParent());
      ExitArgs.push_back(ConstantInt::get(Int32Ty, size));
      ExitArgs.push_back(&args);
    }
  }
}

void InstrumentFunction(std::vector<Value *> MetaData,
                        Function *CurrentFunction, Function *HookedFunction,
                        const CallInst *call, BasicBlock *B) {
  IRBuilder<> Builder(B);

  // Step 1: add space on the heap for pointers
  size_t input_cpt = 0, output_cpt = 0;
  std::vector<Value *> InputAlloca, OutputAlloca;

  allocateMemoryForPointers(CurrentFunction, HookedFunction, Builder,
                            InputAlloca, OutputAlloca, input_cpt, output_cpt,
                            call);

  std::vector<Value *> InputMetaData = MetaData;
  InputMetaData.push_back(ConstantInt::get(Builder.getInt32Ty(), input_cpt));

  std::vector<Value *> OutputMetaData = MetaData;
  OutputMetaData.push_back(ConstantInt::get(Builder.getInt32Ty(), output_cpt));

  // Step 2: for each function input (arguments), add its type, size, name and
  // address to the list of parameters sent to vfc_enter for processing.
  std::vector<Value *> EnterArgs = InputMetaData;
  initializeInputArgs(EnterArgs, CurrentFunction, HookedFunction, call, Builder,
                      InputAlloca);

  // Step 3: call vfc_enter
  Builder.CreateCall(func_enter, EnterArgs);

  // Step 4: load modified values
  std::vector<Value *> FunctionArgs;
  size_t input_index = 0;
  for (auto &args : CurrentFunction->args()) {
    if (args.getType() == FloatTy or args.getType() == DoubleTy) {
      FunctionArgs.push_back(
          Builder.CreateLoad(args.getType(), InputAlloca[input_index++]));
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
  initializeOutputArgs(ExitArgs, CurrentFunction, HookedFunction, ret, call,
                       Builder, OutputAlloca);

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

bool isLLVMDebugFunction(Function &F) {
  StringRef name = F.getName();
  return STARTS_WITH(name, "llvm.dbg.") || STARTS_WITH(name, "llvm.lifetime.");
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
    DoubleTy = Type::getDoubleTy(M.getContext());
    Int8Ty = Type::getInt8Ty(M.getContext());
    Int32Ty = Type::getInt32Ty(M.getContext());
#if LLVM_VERSION_MAJOR < 20
    FloatPtrTy = FloatTy->getPointerTo();
    DoublePtrTy = DoubleTy->getPointerTo();
    Int8PtrTy = Int8Ty->getPointerTo();
#else
    FloatPtrTy = PointerType::getUnqual(FloatTy);
    DoublePtrTy = PointerType::getUnqual(DoubleTy);
    Int8PtrTy = PointerType::getUnqual(Int8Ty);
#endif

    Types2val[FFLOAT] = ConstantInt::get(Int32Ty, FFLOAT);
    Types2val[FDOUBLE] = ConstantInt::get(Int32Ty, FDOUBLE);
    Types2val[FFLOAT_PTR] = ConstantInt::get(Int32Ty, FFLOAT_PTR);
    Types2val[FDOUBLE_PTR] = ConstantInt::get(Int32Ty, FDOUBLE_PTR);

    /*************************************************************************
     *                  Get original functions's names                       *
     *************************************************************************/
    for (auto &F : M) {
      if (F.getName().str() != "main" && F.size() != 0) {
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

    // void vfc_exit_function (char*, char, char, char, char, int, ...)
    func_exit = Function::Create(FunTy, Function::ExternalLinkage,
                                 "vfc_exit_function", &M);

    /*************************************************************************
     *                             Main special case                         *
     *************************************************************************/
    if (M.getFunction("main") != nullptr) {
      Function *Main = M.getFunction("main");

      ValueToValueMapTy VMap;
      Function *Clone = CloneFunction(Main, VMap);

      DISubprogram *Sub = Main->getSubprogram();
      std::string Name = Main->getName().str();
      std::string File = Sub->getFilename().str();
      std::string Line = std::to_string(Sub->getLine());
      std::string NewName = "vfc_" + File + "//" + Name + "/" + Line + "/" +
                            std::to_string(inst_cpt) + "_hook";
      std::string FunctionName =
          File + "//" + Name + "/" + Line + "/" + std::to_string(inst_cpt);
      inst_cpt++;

      bool use_float, use_double;

      // Test if the function use double or float
      haveFloatingPointArithmetic(NULL, Main, &use_float, &use_double);

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

      InstrumentFunction(MetaData, Main, Clone, NULL, block);

      OriginalFunctions.push_back(Clone);
    }

    /*************************************************************************
     *                             Function calls                            *
     *************************************************************************/
    for (auto &F : OriginalFunctions) {
      if (F->getSubprogram() != nullptr) {
        std::string Parent = F->getSubprogram()->getName().str();
        for (auto &B : (*F)) {
          IRBuilder<> Builder(&B);

          for (auto ii = B.begin(); ii != B.end();) {
            Instruction *pi = &(*ii++);

            if (isa<CallInst>(pi)) {
              // collect metadata info //
              if (Function *f = cast<CallInst>(pi)->getCalledFunction()) {
                if (isLLVMDebugFunction(*f)) {
                  continue;
                }

                if (MDNode *N = pi->getMetadata("dbg")) {
                  DILocation *Loc = cast<DILocation>(N);
                  unsigned line = Loc->getLine();
                  std::string File = Loc->getFilename().str();
                  std::string Name;

                  Name = f->getName().str();

                  std::string Line = std::to_string(line);
                  std::string NewName = "vfc_" + File + "/" + Parent + "/" +
                                        Name + "/" + Line + "/" +
                                        std::to_string(inst_cpt) + +"_hook";

                  std::string FunctionName = File + "/" + Parent + "/" + Name +
                                             "/" + Line + "/" +
                                             std::to_string(inst_cpt);
                  inst_cpt++;
                  // Test if f is a library function //
                  const TargetLibraryInfo &TLI = TLIWP.getTLI(*f);

                  _LibFunc libfunc;

                  bool is_from_library = TLI.getLibFunc(f->getName(), libfunc);

                  // Test if f is instrinsic //
                  bool is_intrinsic = f->isIntrinsic();

                  // Test if the function use double or float
                  bool use_float, use_double;
                  haveFloatingPointArithmetic(pi, f, &use_float, &use_double);

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
                                     block);
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

namespace {
struct VfclibFuncPass : public PassInfoMixin<VfclibFuncPass> {
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &) {
    VfclibFunc legacy;
    legacy.runOnModule(M);
    return PreservedAnalyses::none();
  }
};
} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "vfclibfunc", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "vfclibfunc") {
                    MPM.addPass(VfclibFuncPass());
                    return true;
                  }
                  return false;
                });
          }};
}
