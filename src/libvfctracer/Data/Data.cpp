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
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <sstream>
#include <string>

#include <fstream>
#include <iostream>
#include <list>
#include <set>
#include <sstream>
#include <unordered_map>

#include "../opcode.hxx"
#include "../vfctracer.hxx"
#include "Data.hxx"

namespace vfctracerData {

using namespace llvm;

const std::string locInfoSeparator = ";";
  
Data::Data(Instruction *I, DataId id) : Id(id) {
  data = I;
  dataName = "";
  dataRawName = "";
  originalLine = "";
  baseTypeName = "";
  BB = I->getParent();
  F = BB->getParent();
  M = F->getParent();
  operationCode = opcode::getOpCode(data);
  
  switch (operationCode) {
  case opcode::Fops::STORE:
    baseType = data->getOperand(0)->getType();
    basePointerType = data->getOperand(1)->getType();
    break;
  case opcode::Fops::RETURN:
    if (data->getNumOperands() != 0) {
      baseType = data->getOperand(0)->getType();
      basePointerType = baseType->getPointerTo();
    } else { /* data = ret void */
      baseType = data->getType();
      basePointerType = nullptr;
    }
    break;
  case opcode::Fops::CALLINST:
    {
    llvm::CallInst *callInst = dyn_cast<llvm::CallInst>(I);
    baseType = callInst->getArgOperand(0)->getType();
    basePointerType = callInst->getArgOperand(0)->getType();
    }
    break;    
  default:
    baseType = data->getType();
    basePointerType = baseType->getPointerTo();
  }
}

Module* Data::getModule() {
  return M;
}

Function* Data::getFunction() {
  return F;
}

void Data::dump() {
  if (isa<vfctracerData::ScalarData>(this))
    errs() << "[ScalarData]\n";
  else if (isa<vfctracerData::VectorData>(this))
    errs() << "[VectorData]\n";
  else if (isa<vfctracerData::ProbeData>(this))
    errs() << "[ProbeData]\n";
  else if (isa<vfctracerData::PredicateData>(this))
    errs() << "[PredicateData]\n";
    
  errs() << "Data: " << *getData() << "\n"
         << "OpCode: " << opcode::fops_str(opcode::getOpCode(data)) << "\n"
         << "RawName: " << getRawName() << "\n"
         << "Value: " << *getValue() << "\n"
         << "Type: " << *getDataType() << "\n"
         << "Line: " << getOriginalLine() << "\n"
         << "Function: " << getFunctionName() << "\n"
         << "VariableName: " << getVariableName() << "\n";
  std::string sep(80, '-');
  errs() << sep << "\n";
}

bool Data::isTemporaryVariable() const {
  return this->dataName.empty() ||
         this->dataName == vfctracer::temporaryVariableName;
}
  
void Data::findOriginalLine() {
  originalLine = vfctracer::temporaryVariableName;
  locInfo = LocationInfo(*getData());
  originalLine = locInfo.toString();
}
  
std::string Data::getOriginalLine() const {
  return originalLine;
}
  
Instruction *Data::getData() const { return data; }

Value *Data::getValue() const {
  if (opcode::isFPOp(data))
    return data;
  else if (opcode::isStoreOp(data))
    return data->getOperand(0);
  else if (opcode::isRetOp(data))
    return data->getOperand(0);
  else
    llvm_unreachable("Operation unknown");
}

Type *Data::getDataType() const { return baseType; }

Type *Data::getDataPtrType() const { return basePointerType; }

std::string Data::getFunctionName() const { return F->getName().str(); } 

void Data::findRawName() {
  dataRawName = vfctracer::getRawName(data);
}
  
std::string Data::getRawName() const {
  return dataRawName;
}

bool Data::isValidOperation() const {
  Instruction *I = getData();
  /* Checks that the stored value is not a constant */
  if (opcode::isStoreOp(I))
    return not isa<llvm::ConstantFP>(getValue());
  /* Checks that the returned value type is not void */
  if (opcode::isRetOp(I))
    return not getDataType()->isVoidTy();
  return not opcode::isIgnoreOp(I);
}

bool Data::isValidDataType() const {
  if (baseType->isFloatTy())
    return true;
  else if (baseType->isDoubleTy())
    return true;
  else if (llvm::PointerType *PtrTy = dyn_cast<llvm::PointerType>(baseType)) {
    if (PtrTy->getElementType()->isFloatTy())
      return true;
    else if (PtrTy->getElementType()->isDoubleTy())
      return true;
    else
      return false;
  }
  else
    return false;
}

const vfctracerLocInfo::LocationInfo& Data::getLocInfo() const {
  return locInfo;
}

std::string getLocInfoStr(const Data &D) {
  std::string locInfo = D.getDataTypeName() + locInfoSeparator
    + D.getFunctionName() + locInfoSeparator
    + D.getOriginalLine() + locInfoSeparator
    + D.getVariableName();
  return locInfo;
}

  
uint64_t Data::getOrInsertLocInfoValue(std::string ext) {
  std::string rawName = getRawName();
  const std::string locInfoExt = getLocInfo().toString() + ext;
  uint64_t hashLocInfo = vfctracerLocInfo::locInfoHasher(locInfoExt + rawName);
  vfctracerLocInfo::locInfoMap[hashLocInfo] = locInfoExt;
  return hashLocInfo;  
}
  
/* Smart constructor */
Data *CreateData(Instruction *I) {
  /* Checks if instruction is well formed */
  if (I->getParent() == nullptr)
    return nullptr; /* Instruction is not currently inserted into a BasicBlock
                       */
  if (I->getParent()->getParent() == nullptr)
    return nullptr; /* Instruction is not currently inserted into a function*/

  /* Avoid returning call instruction other than vfc_probe */
  if (opcode::isCallOp(I) && not opcode::isProbeOp(I))
    return nullptr;
  
  Data * D = nullptr;
  
  if (opcode::isVectorOp(I))
    D = new vfctracerData::VectorData(I);
  else if (opcode::isProbeOp(I))
    D = new vfctracerData::ProbeData(I);
  else if (opcode::isPredicateOp(I))
    D = new vfctracerData::PredicateData(I);
  else
    D = new vfctracerData::ScalarData(I);

  if (D->isValidOperation() and D->isValidDataType()) {
    /* Search loc info after be sure that is a valid data type */
    D->findOriginalLine();
    return D;
  }
  else
    return nullptr;
}
}
