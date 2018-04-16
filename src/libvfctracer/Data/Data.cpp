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
#include "llvm/Support/raw_ostream.h"

#include <set>
#include <fstream>
#include <unordered_map>
#include <list>
#include <iostream>
#include <sstream>

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

#include "Data.hxx"
#include "../vfctracer.hxx"

namespace vfctracerData {

  using namespace llvm;
  
  Data::Data(Instruction *I) {
    data = I;
    dataName = "";
    dataRawName = "";
    originalLine = "";
    baseTypeName = "";
    BB = I->getParent();
    F = BB->getParent();
    M = F->getParent();          
    operationCode = opcode::getOpCode(*data);
    
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
    default:
      baseType = data->getType();
      basePointerType = baseType->getPointerTo();
    }
  };
    
  void Data::dump() {
    errs() << "Data: " << *getData() << "\n"
	   << "OpCode: " << opcode::fops_str(opcode::getOpCode(data)) << "\n"
	   << "RawName: " << getRawName() << "\n"
	   << "Value: " << *getValue()  << "\n"
	   << "Type: " << *getDataType()   << "\n"
	   << "Line: " << getOriginalLine()  << "\n"
	   << "Function: " << getFunctionName()  << "\n"
	   << "VariableName: " << getVariableName()  << "\n";    
    std::string sep(80,'-');
    errs() << sep << "\n";
  };

  bool Data::isTemporaryVariable() const {
    return this->dataName.empty()
      || this->dataName == vfctracer::temporaryVariableName;
  };

  void printOriginalLine(std::string &OriginalLine, int line) {
    // errs() << "originalLine" << line << " " << OriginalLine << "\n";      
  }
  
  std::string Data::getOriginalLine() {
    if (not originalLine.empty())
      return originalLine;
    
    originalLine = vfctracer::temporaryVariableName;

    Instruction *data = getData();
    if (opcode::isRetOp(data)) {
      
    }
    
    if (opcode::isStoreOp(data)){
      if (const MDNode *N = vfctracer::findVar(data->getOperand(1),F)) {	
	/* Try to get information about the address variable */
	DIVariable Loc(N);
	unsigned line = Loc.getLineNumber();
	originalLine = std::to_string(line);
	printOriginalLine(originalLine,2);
      }
    } else {    
      if (MDNode *N = data->getMetadata(LLVMContext::MD_dbg)) {
	DILocation Loc(N);
	std::string Line = std::to_string(Loc.getLineNumber());
	std::string Column = std::to_string(Loc.getColumnNumber());
	std::string File = Loc.getFilename();
	std::string Dir = Loc.getDirectory();
	originalLine = File + " " + Line + "." + Column;
	printOriginalLine(originalLine,3);
      }
    }
    return originalLine;
  };
        
  Instruction* Data::getData() const {
    return data;
  };

  Value* Data::getValue() const {
    if (opcode::isFPOp(data))
      return data;
    else if (opcode::isStoreOp(data))
      return data->getOperand(0);
    else if (opcode::isRetOp(data))
      return data->getOperand(0);
    else
      llvm_unreachable("Operation unknow");
  };
  Type* Data::getDataType() const {
    return baseType;
  };
  Type* Data::getDataPtrType() const {
    return basePointerType;
  };
  std::string Data::getFunctionName() {
    return F->getName().str();
  };

  std::string& Data::getRawName() {
    if (not dataRawName.empty()) return dataRawName;
    dataRawName = vfctracer::getRawName(data);
    return dataRawName;
  };
  
  bool Data::isValidOperation() {
    return vfctracer::isValidOperation(*this);
  };

  bool Data::isValidDataType() {  
    return vfctracer::isValidDataType(this->getDataType());
  };

  Data* CreateData(Instruction *I) {
    if (I->getType()->isVectorTy())
      return new vfctracerData::VectorData(I);
    else
      return new vfctracerData::ScalarData(I);
  };
  
}
