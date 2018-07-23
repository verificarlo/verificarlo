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

#include "opcode.hxx"
#include "vfctracer.hxx"

#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace opcode {

Fops getOpCode(const Instruction &I) {
  switch (I.getOpcode()) {
  case Instruction::FAdd:
    return Fops::FOP_ADD;
  case Instruction::FSub:
    // In LLVM IR the FSub instruction is used to represent FNeg
    return Fops::FOP_SUB;
  case Instruction::FMul:
    return Fops::FOP_MUL;
  case Instruction::FDiv:
    return Fops::FOP_DIV;
  case Instruction::Store:
    return Fops::STORE;
  case Instruction::Ret:
    return Fops::RETURN;
  // case Instruction::Alloca:
  //   return Fops::ALLOCA;
  case Instruction::Call:
    return Fops::CALLINST;
  default:
    return Fops::FOP_IGNORE;
  }
}

Fops getOpCode(const Instruction *I) {
  switch (I->getOpcode()) {
  case Instruction::FAdd:
    return Fops::FOP_ADD;
  case Instruction::FSub:
    // In LLVM IR the FSub instruction is used to represent FNeg
    return Fops::FOP_SUB;
  case Instruction::FMul:
    return Fops::FOP_MUL;
  case Instruction::FDiv:
    return Fops::FOP_DIV;
  case Instruction::Store:
    return Fops::STORE;
  case Instruction::Ret:
    return Fops::RETURN;
  // case Instruction::Alloca:
  //   return Fops::ALLOCA;
  case Instruction::Call:
    return Fops::CALLINST;
  default:
    return Fops::FOP_IGNORE;
  }
}

std::string getOpStr(const Instruction *I) {
  switch (I->getOpcode()) {
  case Instruction::FAdd:
    return "+";
  case Instruction::FSub:
    // In LLVM IR the FSub instruction is used to represent FNeg
    return "-";
  case Instruction::FMul:
    return "*";
  case Instruction::FDiv:
    return "/";
  case Instruction::Store:
    return "<-";
  default:
    return "";
  }
}

bool isFPOp(Fops op) {
  switch (op) {
  case Fops::FOP_ADD:
  case Fops::FOP_MUL:
  case Fops::FOP_SUB:
  case Fops::FOP_DIV:
    return true;
  default:
    return false;
  }
}

bool isFPOp(Instruction &I) {
  switch (I.getOpcode()) {
  case Instruction::FAdd:
  case Instruction::FSub:
  case Instruction::FMul:
  case Instruction::FDiv:
    return true;
  default:
    return false;
  }
}

bool isFPOp(const Instruction *I) {
  switch (I->getOpcode()) {
  case Instruction::FAdd:
  case Instruction::FSub:
  case Instruction::FMul:
  case Instruction::FDiv:
    return true;
  default:
    return false;
  }
}

bool isStoreOp(Instruction &I) { return I.getOpcode() == Instruction::Store; }

bool isStoreOp(const Instruction *I) {
  return I->getOpcode() == Instruction::Store;
}

bool isRetOp(Instruction &I) { return I.getOpcode() == Instruction::Ret; }

bool isRetOp(const Instruction *I) {
  return I->getOpcode() == Instruction::Ret;
}

bool isIgnoreOp(const Instruction &I) {
  return opcode::getOpCode(I) == Fops::FOP_IGNORE;
}

bool isIgnoreOp(const Instruction *I) {
  return opcode::getOpCode(I) == Fops::FOP_IGNORE;
}

bool isVectorOp(Instruction &I) {
  Type *ty = nullptr;
  if (isStoreOp(I) or isRetOp(I))
    ty = I.getOperand(0)->getType();
  else
    ty = I.getType();
  return ty->isVectorTy();
}

bool isVectorOp(const Instruction *I) {
  Type *ty = nullptr;
  if (isStoreOp(I) or isRetOp(I))
    ty = I->getOperand(0)->getType();
  else
    ty = I->getType();
  return ty->isVectorTy();
}

bool isCallOp(const Instruction *I) {
  return isa<llvm::CallInst>(I);
}

bool isCallOp(const Instruction &I) {
  return isa<llvm::CallInst>(I);
}
  
bool isProbeOp(const Instruction *I) {
  if (const CallInst *callInst = dyn_cast<CallInst>(I)) 
    if (Function *fun = callInst->getCalledFunction())
      return isCallOp(I) && (fun->getName().find(vfctracer::vfcProbeName) != std::string::npos);
  return false;  
}

bool isProbeOp(const Instruction &I) {
  if (const CallInst *callInst = dyn_cast<CallInst>(&I)) 
    if (Function *fun = callInst->getCalledFunction())
      return isCallOp(I) && (fun->getName().find(vfctracer::vfcProbeName) != std::string::npos);
  return false;
}

bool isCallFunOp(const Instruction *I, const std::string & functionName) {
  if (const CallInst *callInst = dyn_cast<CallInst>(I)) {
    Function *fun = callInst->getCalledFunction();
    return isCallOp(I) && fun->getName() == functionName;
  } else {
    return false;
  }
}

bool isCallFunOp(const Instruction &I, const std::string & functionName) {
  if (const CallInst *callInst = dyn_cast<CallInst>(&I)) {
    Function *fun = callInst->getCalledFunction();
    return isCallOp(I) && fun->getName() == functionName;
  } else {
    return false;
  }  
}

std::string fops_str(Fops op) {
  switch (op) {
  case Fops::FOP_ADD:
    return "FOP_ADD";
  case Fops::FOP_SUB:
    return "FOP_SUB";
  case Fops::FOP_MUL:
    return "FOP_MUL";
  case Fops::FOP_DIV:
    return "FOP_DIV";
  case Fops::STORE:
    return "STORE";
  case Fops::RETURN:
    return "RETURN";
  case Fops::ALLOCA:
    return "ALLOCA";
  case Fops::CALLINST:
    return "CALLINST";
  case Fops::FOP_IGNORE:
    return "IGNORE";
  default:
    llvm_unreachable("Bad Fops");
  }
}
}
