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
#include "llvm/IR/GlobalVariable.h"
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

#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR <= 6
#define CREATE_CALL2(func, op1, op2) (builder.CreateCall2(func, op1, op2, ""))
#define CREATE_CALL3(func, op1, op2, op3)                                      \
  (builder.CreateCall3(func, op1, op2, op3, ""))
#define CREATE_STRUCT_GEP(i, p) (builder.CreateStructGEP(i, p))
#else
#define CREATE_CALL2(func, op1, op2) (builder.CreateCall(func, {op1, op2}, ""))
#define CREATE_CALL3(func, op1, op2, op3)                                      \
  (builder.CreateCall(func, {op1, op2, op3}, ""))
#define CREATE_STRUCT_GEP(i, p) (builder.CreateStructGEP(nullptr, i, p, ""))
#endif

#include "../vfctracer.hxx"
#include "Format.hxx"

using namespace llvm;
using namespace opcode;
using namespace vfctracer;
using namespace vfctracerData;
using namespace vfctracerLocInfo;

namespace vfctracerFormat {

Constant *TextFmt::CreateProbeFunctionPrototype(Data &D) {

  const std::string dataTypeName = D.getDataTypeName();
  std::string probeFunctionName = vfctracer::probePrefixName +
    dataTypeName;
  
  if (isa<vfctracerData::ProbeData>(D))
    probeFunctionName += "_ptr";
  
  Type *returnType = Type::getVoidTy(M->getContext());
  Type *valueType = D.getDataType();
  Type *valuePtrType = D.getDataPtrType();
  Type *locInfoType = getLocInfoType(D);
  Constant *probeFunc =
      M->getOrInsertFunction(probeFunctionName, returnType, valueType,
                             valuePtrType, locInfoType, (Type *)0);

  return probeFunc;
}

CallInst *TextFmt::InsertProbeFunctionCall(Data &D, Value *probeFunc) {
  
  // We instrument condition of predicate
  Instruction *I = nullptr;
  if (PredicateData *PredD = dyn_cast<PredicateData>(&D))
    I = cast<Instruction>(PredD->getCondition());
  else
    I = D.getData();
  
  IRBuilder<> builder(I);
  /* For FP operations, we need to insert the probe after the instruction */
  if (opcode::isFPOp(I) or isa<PredicateData>(D))
    builder.SetInsertPoint(I->getNextNode());

  IntegerType *i1Ty = IntegerType::get(D.getModule()->getContext(), 1);
  
  Value *value = D.getValue();
  if (value->getType() != D.getDataType() and value->getType() != i1Ty) {
    errs() << "Value type and Data type do not match\n";
    errs() << *value->getType() << " != " << *D.getDataType() << "\n";
  }

  Value *valuePtr = D.getAddress();
  Value *locInfoValue = getOrCreateLocInfoValue(D);
  CallInst *callInst = nullptr;
  callInst = CREATE_CALL3(probeFunc, value, valuePtr, locInfoValue);
  return callInst;
}

Type *TextFmt::getLocInfoType(Data &D) {
  if (isa<ScalarData>(D) or isa<ProbeData>(D) or isa<PredicateData>(D)) {
    return Type::getInt64Ty(M->getContext());
  } else if (VectorData *VD = dyn_cast<VectorData>(&D)) {
    unsigned vectorSize = VD->getVectorSize();
    Type *int64Ty = Type::getInt64Ty(M->getContext());
    ArrayType *arrayLocInfoType = ArrayType::get(int64Ty, vectorSize);
    return PointerType::get(arrayLocInfoType, 0);
  } else {    
    llvm_unreachable("Unknow Data type");
  }
}

Value *TextFmt::getOrCreateLocInfoValue(Data &D) {
  if (isa<ScalarData>(D) or isa<ProbeData>(D) or isa<PredicateData>(D)) {
    uint64_t keyLocInfo = D.getOrInsertLocInfoValue();
    Type *int64Ty = Type::getInt64Ty(M->getContext());
    Constant *locInfoValue = ConstantInt::get(int64Ty, keyLocInfo, false);
    return locInfoValue;
  } else if (VectorData *VD = dyn_cast<VectorData>(&D)) {
    std::string locInfoGVname = "arrayLocInfoGV." + VD->getVariableName() + "." + VD->getRawName();
    GlobalVariable *arrayLocInfoGV = M->getGlobalVariable(locInfoGVname);
    Type *int64Ty = Type::getInt64Ty(M->getContext());
    if (arrayLocInfoGV == nullptr) {
      /* Create vector of locationInfo keys */
      std::vector<Constant *> locInfoKeyVector;
      std::vector<uint64_t> locInfoHashVector = VD->getOrInsertLocInfoValue();
      for (auto &hash : locInfoHashVector) {
        Constant *locInfoValue = ConstantInt::get(int64Ty, hash, false);
        locInfoKeyVector.push_back(locInfoValue);
      }
      /* Create Globale Variable which contains the constant array */
      ArrayType *arrayLocInfoType =
          ArrayType::get(int64Ty, VD->getVectorSize());
      /* Constant Array containing locationInfo keys */
      Constant *constArrayLocInfo =
          ConstantArray::get(arrayLocInfoType, locInfoKeyVector);
      arrayLocInfoGV =
          new GlobalVariable(/*Module=*/*M,
                             /*Type=*/arrayLocInfoType,
                             /*isConstant=*/true,
                             /*Linkage=*/GlobalValue::ExternalLinkage,
                             /*Initializer=*/constArrayLocInfo,
                             /*Name=*/locInfoGVname);
    }
    return arrayLocInfoGV;
  } else {
    llvm_unreachable("Unknow Data class");
  }
}

}
