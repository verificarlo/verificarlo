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
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/TypeBuilder.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/User.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/ADT/Twine.h"
#include "llvm/ADT/APFloat.h"

#include <set>
#include <fstream>
#include <unordered_map>
#include <list>

#if LLVM_VERSION_MINOR >= 5
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

#include "Data.hxx"
#include "../vfctracer.hxx"

using namespace llvm;
using namespace opcode;
using namespace vfctracer;

namespace vfctracerData {
  
  ScalarData::ScalarData(Instruction *I) : Data(I) {}

  Value* ScalarData::getAddress() const {  
    Instruction *I = getData();
    switch (operationCode){
    case Fops::STORE:
      return I->getOperand(1);
    default:
      /* Temporary FP arithmetic instructions don't have memory address */
      PointerType *ptrTy = getDataType()->getPointerTo();
      ConstantPointerNull *nullptrValue = ConstantPointerNull::get(ptrTy);
      return nullptrValue;
    }
  }
    
  std::string ScalarData::getOriginalName(const Value *V) {
    // If the value is defined as a GetElementPtrInstruction,
    // return the name of the pointer operand instead
    if (const GetElementPtrInst *I = dyn_cast<GetElementPtrInst>(V)) {
      return getOriginalName(I->getPointerOperand());
    }
    // If the value is a constant Expr,
    // such as a GetElementPtrInstConstantExpr,
    // try to get the name of its first operand
    if (const ConstantExpr *E = dyn_cast<ConstantExpr>(V)) {
      return getOriginalName(E->getOperand(0));
    }

    std::string name = vfctracer::findName(V);
    if (name != vfctracer::temporaryVariableName)  return name;
    	           
    std::vector<std::string> nameVector;
    /* Check wether one of the operands have a name */
    if (const Instruction *I = dyn_cast<Instruction>(V)) {
      for (unsigned int i = 0 ; i < I->getNumOperands(); ++i) {
	Value *v = I->getOperand(i);	  
	std::string name = vfctracer::findName(v);
	if (name != vfctracer::temporaryVariableName) nameVector.push_back(name);
      }
      std::string to_return = (nameVector.empty()) ? "" : nameVector.front();
      for (unsigned int i = 1; i < nameVector.size(); i++)
	to_return += "," + nameVector[i];

      if (not to_return.empty()) return to_return;
    }      
    return vfctracer::temporaryVariableName;      
  }
    
  std::string ScalarData::getVariableName() {
    // check whether the name has already been found
    if (not dataName.empty())
      return dataName;

    if (operationCode == Fops::STORE) {   
      dataName = getOriginalName(data->getOperand(0));      
    } else
      dataName = getOriginalName(data);

    return dataName;	
  }
    
  std::string ScalarData::getDataTypeName() {
    if (baseTypeName.empty()) 
      baseTypeName = vfctracer::getBaseTypeName(baseType);
    return baseTypeName;
  } 

}
