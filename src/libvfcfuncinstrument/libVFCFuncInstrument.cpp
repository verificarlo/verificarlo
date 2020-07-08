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
#include "llvm/IR/CallSite.h"
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
#include "llvm/Transforms/Utils/UnifyFunctionExitNodes.h"

#include <cxxabi.h>
#include <fstream>
#include <iostream>
#include <set>
#include <stdio.h>
#include <utility>
#include <vector>

using namespace llvm;

namespace {

static Type *Int8PtrTy;
static Type *Int8Ty;
static Type *Int32Ty;
static Type *Int64Ty;
static Type *FloatTy;
static Type *DoubleTy;
static Type *VoidTy;
static Function *func_enter;
static Function *func_exit;

// Enumeration of managed types
enum Ftypes { FLOAT, DOUBLE };

struct VfclibFunc : public ModulePass {
  static char ID;

  VfclibFunc() : ModulePass(ID) {}

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<TargetLibraryInfoWrapperPass>();
    AU.addRequired<UnifyFunctionExitNodes>();
  }

  // Fill use_double and use_float with true if the call_inst pi use at least
  // of the managed types
  void haveFloatingPointArithmetic(Instruction *call, Function *f,
                                   bool is_from_library, bool is_intrinsic,
                                   bool *use_float, bool *use_double) {
    Type *ReturnTy;

    if (f) {
      ReturnTy = f->getReturnType();
    } else {
      ReturnTy = call->getType();
      ;
    }

    // Test if return type of call is float
    (*use_float) = ReturnTy == FloatTy;

    // Test if return type of call is double
    (*use_double) = ReturnTy == DoubleTy;

    // Test if f treat floats point numbers
    if (is_intrinsic or is_from_library) {
      // Loop over arguments types
      for (auto it = call->op_begin(); it < call->op_end() - 1; it++) {
        if ((*it)->getType() == FloatTy)
          (*use_float) = true;
        if ((*it)->getType() == DoubleTy)
          (*use_double) = true;
      }
    } else {
      // Loop over each instruction of the function and test if one of them
      // use float or double
      for (auto &bbi : (*f)) {
        for (auto &ii : bbi) {
          Type *opType = ii.getOperand(0)->getType();

          if (opType->isVectorTy()) {
            VectorType *t = static_cast<VectorType *>(opType);
            opType = t->getElementType();
          }

          if (opType == FloatTy)
            (*use_float) = true;

          if (opType == DoubleTy)
            (*use_double) = true;
        }

        if ((*use_float) or (*use_double))
          break;
      }
    }
  }

  // Demangling function
  std::string demangle(std::string src) {
    int status = 0;
    char *demangled_name = NULL;
    if ((demangled_name = abi::__cxa_demangle(src.c_str(), 0, 0, &status))) {
      src = demangled_name;
      std::size_t first = src.find("(");
      src = src.substr(0, first);
    }
    free(demangled_name);

    return src;
  }

  AllocaInst *allocatePtr(BasicBlock &B, Type *VType,
                          std::string FunctionName) {
    static size_t cpt = 0;

    AllocaInst *ptr = new AllocaInst(
        VType, 0, 0, FunctionName + "_arg_" + std::to_string(cpt++));

    for (auto &ii : B){
      if(!isa<PHINode>(ii)){
        ptr->insertBefore(&ii);
        break;
      }
    }

    return ptr;
  }

  Type *fillArgs(bool is_enter, std::vector<Value *> &Args, Value *v,
                 Value **Types2val, int &cpt) {
    Type *VType = NULL;
    Type *T = v->getType();

    if (T == FloatTy) {
      // Push the typeID
      VType = FloatTy;
      Args.push_back(Types2val[FLOAT]);

      // Number of argument added
      cpt++;

    } else if (T == DoubleTy) {
      // Push the typeID
      VType = DoubleTy;
      Args.push_back(Types2val[DOUBLE]);

      // Number of argument added
      cpt++;
    }

    return VType;
  }

  void mainInstrumentation(Module &M) {
    Function *F = M.getFunction("main");

    if (F == NULL) {
      std::string Mangled;
      raw_string_ostream mangledNameStream(Mangled);
      Mangler::getNameWithPrefix(mangledNameStream, "main", M.getDataLayout());
      mangledNameStream.flush();
      F = M.getFunction(Mangled);
      if (F == NULL) {
        return;
      }
    }

    DISubprogram *Sub = F->getSubprogram();

    std::string main_id = Sub->getFilename().str() + "/" +
                          Sub->getName().str() + "_" +
                          std::to_string(Sub->getLine());

    IRBuilder<> Builder(&(*F->begin()));

    Value *MainID = Builder.CreateGlobalStringPtr(main_id);

    // Metadata Info
    bool is_from_library = 0;
    bool is_intrinsic = 0;
    bool use_float, use_double;

    haveFloatingPointArithmetic(NULL, F, is_from_library, is_intrinsic,
                                &use_float, &use_double);

    // Constants creation
    Constant *isLibraryFunction = ConstantInt::get(Int8Ty, is_from_library);
    Constant *isInstrinsicFunction = ConstantInt::get(Int8Ty, is_intrinsic);
    Constant *haveFloat = ConstantInt::get(Int8Ty, use_float);
    Constant *haveDouble = ConstantInt::get(Int8Ty, use_double);
    Constant *zero_const = ConstantInt::get(Int32Ty, 0);

    // Enter function arguments
    std::vector<Value *> MainArgs{
        MainID,    isLibraryFunction, isInstrinsicFunction,
        haveFloat, haveDouble,        zero_const};

    BasicBlock &EntryNode = F->getEntryBlock();

    Builder.SetInsertPoint(&EntryNode, EntryNode.begin());

    Builder.CreateCall(func_enter, MainArgs);

    UnifyFunctionExitNodes pass;

    pass.runOnFunction(*F);

    BasicBlock *ExitNode = pass.getReturnBlock();

    if (ExitNode != nullptr) {
      Instruction *terminator = ExitNode->getTerminator();

      if (terminator != nullptr) {
        Builder.SetInsertPoint(terminator);

        Builder.CreateCall(func_exit, MainArgs);
      }
    }
  }

  /**************************************************************************
   *                     Function Call Instrumentation                      *
   *                                                                        *
   *  // The old function call                                              *
   *  resf = f ( arg1, arg2 )                                               *
   *                                                                        *
   *  // is replaced by the folowing sequence of Instructions               *
   *                                                                        *
   *  // create pointers for arguments                                      *
   *  ptr_arg1 = store arg1                                                 *
   *  ptr_arg2 = store arg2                                                 *
   *                                                                        *
   *  // call to vfc_enter_function                                         *
   *  call void vfc_enter_function( function_ID, nb_param, type_arg1,       *
   *                                 ptr_arg1, type_arg2, ptr_arg2 )        *
   *                                                                        *
   *  // get new arguments values                                           *
   *  new_arg1 = load ptr_arg1                                              *
   *  new_arg2 = load ptr_arg2                                              *
   *                                                                        *
   *  // call to the original function with new arguments                   *
   *  tmp_resf = f ( new_arg1, new_arg2 )                                   *
   *                                                                        *
   *  // create pointer for result                                          *
   *  ptr_tmp_resf = store tmp_resf                                         *
   *                                                                        *
   *  // call to vfc_exit_function                                          *
   *  call void vfc_exit_function(  function_ID, nb_param,                  *
   *                                type_resf, ptr_tmp_resf )               *
   *                                                                        *
   *  // get the new return value                                           *
   *  res_f = load ptr_tmp_resf                                             *
   **************************************************************************/

  virtual bool runOnModule(Module &M) {
    const TargetLibraryInfo *TLI =
        &getAnalysis<TargetLibraryInfoWrapperPass>().getTLI();

    // Types
    Int8PtrTy = Type::getInt8PtrTy(M.getContext());
    Int8Ty = Type::getInt8Ty(M.getContext());
    Int32Ty = Type::getInt32Ty(M.getContext());
    Int64Ty = Type::getInt64Ty(M.getContext());
    FloatTy = Type::getFloatTy(M.getContext());
    DoubleTy = Type::getDoubleTy(M.getContext());
    VoidTy = Type::getVoidTy(M.getContext());

    // Types string ptr
    IRBuilder<> Builder(&(*(*M.begin()).begin()));
    Value *Types2val[] = {ConstantInt::get(Int32Ty, 0),
                          ConstantInt::get(Int32Ty, 1)};

    /*************************************************************************
     *                  Enter and exit functions declarations                *
     *************************************************************************/

    std::vector<Type *> ArgTypes{Int8PtrTy, Int8Ty, Int8Ty,
                                 Int8Ty,    Int8Ty, Int32Ty};

    // void vfc_enter_function (char*, char, char, char, char, int, ...)
    Constant *func = M.getOrInsertFunction(
        "vfc_enter_function", FunctionType::get(VoidTy, ArgTypes, true));

    func_enter = cast<Function>(func);

    // void vfc_exit_function (char*, char, char, char, char, int, ...)
    func = M.getOrInsertFunction("vfc_exit_function",
                                 FunctionType::get(VoidTy, ArgTypes, true));

    func_exit = cast<Function>(func);

    /**************************************************************************
     *                     Function's calls instrumentation                   *
     **************************************************************************/

    for (auto &F : M) {
      for (auto &B : F) {
        IRBuilder<> Builder(&B);

        for (auto ii = B.begin(); ii != B.end();) {
          Instruction *pi = &(*ii++);

          if (isa<CallInst>(pi)) {
            // collect metadata info //
            Function *f = cast<CallInst>(pi)->getCalledFunction();
            MDNode *N = pi->getMetadata("dbg");
            DILocation *Loc = cast<DILocation>(N);
            unsigned Line = Loc->getLine();
            std::string File = Loc->getFilename().str();
            std::string Name = f->getName().str();

            // Test if f is a library function //
            LibFunc libfunc;
            bool is_from_library = TLI->getLibFunc(f->getName(), libfunc);

            // Test if f is instrinsic //
            bool is_intrinsic = f->isIntrinsic();

            bool use_float, use_double;

            // Test if the function use double or float
            haveFloatingPointArithmetic(pi, f, is_from_library, is_intrinsic,
                                        &use_float, &use_double);

            // Demangle the name of F //
            Name = demangle(Name);

            // Create function ID
            std::string FunctionName =
                File + "/" + Name + "_" + std::to_string(Line);
            Value *FunctionID = Builder.CreateGlobalStringPtr(FunctionName);

            // Constants creation
            Constant *isLibraryFunction =
                ConstantInt::get(Int8Ty, is_from_library);
            Constant *isInstrinsicFunction =
                ConstantInt::get(Int8Ty, is_intrinsic);
            Constant *haveFloat = ConstantInt::get(Int8Ty, use_float);
            Constant *haveDouble = ConstantInt::get(Int8Ty, use_double);
            Constant *zero_const = ConstantInt::get(Int32Ty, 0);

            // Enter function arguments
            std::vector<Value *> EnterArgs{
                FunctionID, isLibraryFunction, isInstrinsicFunction,
                haveFloat,  haveDouble,        zero_const};

            // Exit function arguments
            std::vector<Value *> ExitArgs{
                FunctionID, isLibraryFunction, isInstrinsicFunction,
                haveFloat,  haveDouble,        zero_const};

            // Temporary function call to make loads and stores insertions
            // easier
            CallInst *enter_call = CallInst::Create(func_enter, EnterArgs);
            enter_call->insertBefore(pi);

            // The clone of the instrumented function's call will be used with
            // hooked arguments and the original one will be replaced by the
            // load of the hooked return value
            Instruction *func_call = pi->clone();
            func_call->insertBefore(pi);

            // Temporary function call to make loads and stores insertions
            // easier
            CallInst *exit_call = CallInst::Create(func_exit, ExitArgs);
            exit_call->insertBefore(pi);

            // Instrumented function arguments
            std::vector<Value *> FunctionArgs;

            int m = 0;
            // loop over arguments
            for (auto it = pi->op_begin(); it < pi->op_end() - 1; it++) {
              // Get Value
              Value *v = (*it);

              Type *VType = fillArgs(1, EnterArgs, v, Types2val, m);

              if (VType == NULL) {
                FunctionArgs.push_back(v);
                continue;
              }

              // Allocate pointer
              AllocaInst *ptr = allocatePtr(B, VType, FunctionName);

              // Enter store and load
              StoreInst *enter_str = new StoreInst(v, ptr);
              enter_str->insertBefore(enter_call);
              LoadInst *enter_load = new LoadInst(VType, ptr);
              enter_load->insertAfter(enter_call);
              EnterArgs.push_back(ptr);
              FunctionArgs.push_back(enter_load);
            }

            // Values modified by the instumented function
            std::vector<Value *> Returns;
            Returns.push_back(func_call);

            int n = 0;
            for (auto &it : Returns) {
              // Get Value
              Value *v = it;

              Type *VType = fillArgs(0, ExitArgs, v, Types2val, n);

              if (VType == NULL) {
                pi->eraseFromParent();
                continue;
              }

              // Allocate pointer
              AllocaInst *ptr = allocatePtr(B, VType, FunctionName);

              StoreInst *exit_str = new StoreInst(v, ptr);
              exit_str->insertBefore(exit_call);
              LoadInst *exit_load = new LoadInst(VType, ptr);
              ExitArgs.push_back(ptr);

              if (v == func_call) {
                ReplaceInstWithInst(pi, exit_load);
              } else {
                exit_load->insertAfter(exit_call);
              }
            }

            // Replace temporary functions calls by definitive ones
            CallInst *new_enter_call = CallInst::Create(func_enter, EnterArgs);
            ReplaceInstWithInst(enter_call, new_enter_call);

            CallInst *new_exit_call = CallInst::Create(func_exit, ExitArgs);
            ReplaceInstWithInst(exit_call, new_exit_call);

            // Replace with the good number of operand
            Constant *num_operands = ConstantInt::get(Int32Ty, m);
            new_enter_call->setOperand(5, num_operands);

            // Replace with the good number of return value
            Constant *num_results = ConstantInt::get(Int32Ty, n);
            new_exit_call->setOperand(5, num_results);

            CallInst *new_func_call = CallInst::Create(f, FunctionArgs);
            ReplaceInstWithInst(func_call, new_func_call);
          }
        }
      }
    }

    mainInstrumentation(M);

    return true;
  }
};

} // namespace

char VfclibFunc::ID = 0;
static RegisterPass<VfclibFunc>
    X("vfclibfunc", "verificarlo function instrumentation pass", false, false);