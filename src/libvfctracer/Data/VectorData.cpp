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
  
  VectorData::VectorData(Instruction *I) : Data(I) {
        
    vectorType = static_cast<VectorType*>(baseType);
    baseType = vectorType->getElementType();
    vectorSize = vectorType->getNumElements();

    switch (vectorSize) {
    case 2:
      vectorName = vfctracer::vectorName_x2;
      break;
    case 4:
      vectorName = vfctracer::vectorName_x4;
      break;
    default:
      errs() << "Unsupported operand type: " << *vectorType<< "\n";
      assert(0);	  
    }     
  };

  Type* VectorData::getDataType() const {
    return this->vectorType;
  };

  unsigned VectorData::getVectorSize() const {
    return this->vectorSize;
  };

  Value* VectorData::getAddress() const {
    Instruction *I = this->getData();
    Fops operationCode = getOpCode(*I);      
    switch (operationCode){
    case Fops::STORE:
      return I->getOperand(1);
    default:
      /* Temporary FP arithmetic instructions don't have memory address */
      ConstantPointerNull *nullptrValue =
	ConstantPointerNull::get(vectorType->getPointerTo());
      return nullptrValue;
    }
  };
    
  Type* VectorData::getVectorType() const { return this->vectorType; };
    
  std::string VectorData::getOriginalName(const Value *V) {
    /* Vector are accessed through load and bitcast */
    /* We need to check if it is the case and thus */
    /* retrieve the original name attached to the pointer */
    if (const Instruction *I = dyn_cast<Instruction>(V)) {
      for (unsigned int i = 0; i < I->getNumOperands(); ++i) {	
	Value *op = I->getOperand(i); 	  
	if (const LoadInst *Load = dyn_cast<LoadInst>(op)) {
	  Value * ptrOp = Load->getOperand(0);
	  if (const BitCastInst *Bitcast = dyn_cast<BitCastInst>(ptrOp)) {
	    Value *V = Bitcast->getOperand(0);
	    return dynamic_cast<ScalarData*>(this)->getOriginalName(V);
	  }
	}
      }	
    }
    return "";
  }
    
  bool VectorData::isValidDataType() {
    return vfctracer::isValidDataType(this->baseType);
  };

  std::string VectorData::getVariableName() {
    if (not dataName.empty()) return dataName;
      
    dataName = vfctracer::findName(data);
    if (isTemporaryVariable()) dataName = getOriginalName(data);

    return dataName;	
  };     

  std::string VectorData::getDataTypeName() {
    if (this->baseTypeName.empty()) 
      baseTypeName = vectorName + vfctracer::getBaseTypeName(baseType);
    return baseTypeName;
  };
    
}
