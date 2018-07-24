/********************************************************************************
 *                                                                              *
 *  This file is part of Verificarlo.                                           *
 *                                                                              *
 *  Copyright (c) 2018                                                          *
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

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/TypeBuilder.h"
#include "llvm/IR/User.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <sstream>
#include <string>

#include <fstream>
#include <list>
#include <set>
#include <unordered_map>

#if LLVM_VERSION_MINOR <= 6
#define CREATE_CALL2(func, op1, op2) (builder.CreateCall2(func, op1, op2, ""))
#define CREATE_STRUCT_GEP(i, p) (builder.CreateStructGEP(i, p))
#else
#define CREATE_CALL2(func, op1, op2) (builder.CreateCall(func, {op1, op2}, ""))
#define CREATE_STRUCT_GEP(i, p) (builder.CreateStructGEP(nullptr, i, p, ""))
#endif

#include "../vfctracer.hxx"
#include "Data.hxx"

using namespace llvm;
using namespace opcode;
using namespace vfctracer;

namespace vfctracerData {

std::string getStringOfGlobalVariable(llvm::Value *v){
  std::string name;
  llvm::Value *arg_ptr_cast = v->stripPointerCasts();
  if (llvm::GlobalVariable *GVstr = dyn_cast<llvm::GlobalVariable>(arg_ptr_cast)) {
      if (llvm::ConstantDataSequential *cstDataArr =
	  dyn_cast<llvm::ConstantDataSequential>(GVstr->getInitializer())) {
	name = cstDataArr->getAsString();
	if (name.back() == '\0') name.pop_back();
      }
  }
  return name;
}

std::string findGlobalString(llvm::Value *v) {

  std::string name = getStringOfGlobalVariable(v);
  if (not name.empty()) return name;

  for (User *U : v->users()) {
    if (llvm::MemCpyInst *memcopy = dyn_cast<llvm::MemCpyInst>(U)) {
      Value *globalStringGEP = memcopy->getArgOperand(1);
      return getStringOfGlobalVariable(globalStringGEP);
    }
    if (llvm::BitCastInst *bitcast = dyn_cast<llvm::BitCastInst>(U)) {
      name = findGlobalString(bitcast);
      if (not name.empty())
	return name;
    }
  }
  return name;
}

ProbeData::ProbeData(Instruction *I, DataId id) : Data(I, id) {
  CallInst *callInst = dyn_cast<CallInst>(data);  
  llvm::Value *arg1 = callInst->getArgOperand(1);
  if (arg1->getType()->isPointerTy() && arg1->getType()->getPointerElementType()->isIntegerTy(8)){
    dataName = findGlobalString(arg1);
    probe_arg = callInst->getArgOperand(0);
  }
}
 
Value *ProbeData::getAddress() const {
  return getValue();
}

Value *ProbeData::getValue() const {
  return probe_arg;
}
  
Type *ProbeData::getDataType() const {
  return baseType ;
}

bool ProbeData::isValidDataType() const {
  if (llvm::PointerType *PtrTy = dyn_cast<llvm::PointerType>(baseType)) {
    if (PtrTy->getElementType()->isFloatTy())
      return true;
    else if (PtrTy->getElementType()->isDoubleTy())
      return true;
    else
      return false;
  }  
  return false;
}
    
std::string ProbeData::getVariableName() {
  if (not dataName.empty())
    return dataName;
  else
    llvm_unreachable("Name must be found");
}
  
std::string ProbeData::getDataTypeName() {
  if (baseTypeName.empty())
    baseTypeName = vfctracer::getBaseTypeName(baseType);
  return baseTypeName;
}


 
}
