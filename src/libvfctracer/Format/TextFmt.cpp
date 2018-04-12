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

#include "Format.hxx"
#include "../vfctracer.hxx"

using namespace llvm;
using namespace opcode;
using namespace vfctracer;
using namespace vfctracerData;

namespace vfctracerFormat {
  
  TextFmt::TextFmt(Module &M) {
    this->M = &M;
  }

  Constant* TextFmt::CreateProbeFunctionPrototype(Data &D) {
      
    std::string probeFunctionName = vfctracer::probePrefixName + D.getDataTypeName();
    Type *returnType = Type::getVoidTy(M->getContext());
    Type *valueType = D.getDataType();
    Type *valuePtrType = D.getDataPtrType();
    Type *locInfoType = getLocInfoType(D);
    Constant *probeFunc = M->getOrInsertFunction(probeFunctionName,
						 returnType,
						 valueType,
						 valuePtrType,
						 locInfoType,
						 (Type*)0);

    return probeFunc;
  };

  CallInst* TextFmt::InsertProbeFunctionCall(Data &D, Value *probeFunc) {

    IRBuilder<> builder(D.getData());
    /* For FP operations, need to insert the probe after the instruction */
    if (opcode::isFPOp(D.getData()))
      builder.SetInsertPoint(D.getData()->getNextNode());
    Value *value = D.getValue();
    Value *valuePtr = D.getAddress();
    Value *locInfoValue = getOrCreateLocInfoValue(D);
    CallInst *callInst = builder.CreateCall3(cast<Function>(probeFunc),
					     value,
					     valuePtr,
					     locInfoValue,
					     "");

    return callInst;
  };

  Type* TextFmt::getLocInfoType(Data &D) {
    if (typeid(D) == typeid(ScalarData)) {
      return Type::getInt8PtrTy(M->getContext());
    } else if (typeid(D) == typeid(VectorData)) {
      return Type::getInt8PtrTy(M->getContext())->getPointerTo();
    } else {
      assert(0);
    }
  };

  Value* TextFmt::getOrCreateLocInfoValue(Data &D) {
    if (typeid(D) == typeid(ScalarData)) {
    
      IRBuilder<> builder(D.getData());
      Fops opCode = getOpCode(*D.getData());
      /* For FP operations, need to insert the probe after the instruction */
      if (opcode::isFPOp(D.getData()))
	builder.SetInsertPoint(D.getData()->getNextNode());
      std::string locInfo = vfctracer::getLocInfo(D);
      uint64_t keyLocInfo = vfctracer::getOrInsertLocInfoValue(locInfo);
      Value *strPtr = builder.CreateGlobalStringPtr(locInfo, "locInfo.str");
      return strPtr;

    } else if (VectorData* VD = dynamic_cast<VectorData*>(&D)) {

      std::string locInfoGVname = "arrayLocInfoGV." + VD->getVariableName();
      GlobalVariable * arrayLocInfoGV = M->getGlobalVariable(locInfoGVname);
      Type *int64Ty = Type::getInt64Ty(M->getContext());
      errs() << *arrayLocInfoGV << "\n";
      if (arrayLocInfoGV == nullptr) {

	std::string locInfo = vfctracer::getLocInfo(*VD);

	/* Create vector of locationInfo keys */
	std::vector<Constant*> locInfoKeyVector;	  
	for(int i = 0; i < VD->getVectorSize(); ++i) {
	  Constant *strPtr = ConstantDataArray::getString(M->getContext(), locInfo, true);;
	  locInfoKeyVector.push_back(strPtr);
	}
	/* Create Globale Variable which contains the constant array */
	PointerType *strPtrType = Type::getInt8PtrTy(M->getContext());
	ArrayType* arrayLocInfoTy = ArrayType::get(strPtrType, VD->getVectorSize());
	/* Constant Array containing locationInfo keys */
	Constant* constArrayLocInfo = ConstantArray::get(arrayLocInfoTy,
							 locInfoKeyVector);
      
	GlobalVariable* arrayLocInfoGV = new GlobalVariable(/*Module=*/*M, 
							    /*Type=*/arrayLocInfoTy,
							    /*isConstant=*/false,
							    /*Linkage=*/GlobalValue::ExternalLinkage,
							    /*Initializer=*/constArrayLocInfo,
							    /*Name=*/locInfoGVname);
      }
      
      /* Create indices list for accessing to Global Array with getElementPtr */
      Constant* zero_constInt64 = ConstantInt::get(int64Ty, 0);
      std::vector<Constant*> constPtrIndices;
      constPtrIndices.push_back(zero_constInt64);
      constPtrIndices.push_back(zero_constInt64);
      Value *locInfoValue = ConstantExpr::getGetElementPtr(arrayLocInfoGV,
							   constPtrIndices);
      return locInfoValue;

    } else {
      assert(false);
    }
  
  };

}
