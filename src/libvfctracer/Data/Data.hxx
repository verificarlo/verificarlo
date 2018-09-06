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

#ifndef _DATA_DATA_HXX__
#define _DATA_DATA_HXX__

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Casting.h"
#include <string>

#include "../opcode.hxx"
#include "../LocationInfo.hxx"

using namespace vfctracerLocInfo;

namespace vfctracerData {

  enum DataId { ScalarId, ProbeId, VectorId, PredicateId };

class Data {
private:
  const DataId Id;

protected:
  llvm::Module *M;
  llvm::Function *F;
  llvm::BasicBlock *BB;
  llvm::Instruction *data;
  llvm::Type *baseType;
  llvm::Type *basePointerType;
  llvm::Type *locationInfoType;
  std::string dataName;
  std::string dataRawName; /* SSA form name */
  std::string baseTypeName;
  std::string originalLine;
  opcode::Fops operationCode;
  vfctracerLocInfo::LocationInfo locInfo;
  
public:
  Data(llvm::Instruction *I, DataId Id);
  virtual ~Data() = 0;
  virtual llvm::Instruction *getData() const;
  virtual llvm::Value *getValue() const;
  virtual llvm::Type *getDataType() const;
  virtual llvm::Type *getDataPtrType() const;
  virtual bool isTemporaryVariable() const;
  virtual bool isValidOperation() const;
  virtual bool isValidDataType() const;
  virtual void findOriginalLine();
  virtual std::string getOriginalLine() const;
  virtual std::string getFunctionName() const;
  virtual void findRawName();
  virtual std::string getRawName() const ;
  virtual void findDataTypeName() = 0;
  virtual std::string getDataTypeName() const = 0;
  virtual llvm::Value *getAddress() const = 0;
  virtual void findVariableName() = 0;
  virtual std::string getVariableName() const = 0;
  virtual void dump();
  DataId getValueId() const { return Id; };
  llvm::Module* getModule();
  llvm::Function* getFunction();
  const vfctracerLocInfo::LocationInfo& getLocInfo() const ;
  virtual uint64_t getOrInsertLocInfoValue(std::string ext = "");
};

class ScalarData : public Data {
public:
  ScalarData(llvm::Instruction *, DataId id = ScalarId);
  llvm::Value *getAddress() const;
  void findVariableName();
  std::string getVariableName() const;
  void findDataTypeName();
  std::string getDataTypeName() const;
  static inline bool classof(const Data *D) {
    return D->getValueId() == ScalarId;
  }
};

class VectorData : public Data {
private:
  llvm::VectorType *vectorType;
  std::string vectorName;
  unsigned vectorSize;

public:
  VectorData(llvm::Instruction *I, DataId id = VectorId);
  llvm::Value *getAddress() const;
  llvm::Type *getVectorType() const;
  llvm::Type *getDataType() const;
  unsigned getVectorSize() const;
  void findVariableName();
  std::string getVariableName() const ;
  void findDataTypeName();
  std::string getDataTypeName() const;
  static inline bool classof(const Data *D) {
    return D->getValueId() == VectorId;
  }
};

// Call used for vfc_probe added by the user
// data: vfc_probe callinst
// arg: value to trace, argument of vfc_probe
class ProbeData : public Data {
private:
  llvm::Value *probe_arg;
public:
  ProbeData(llvm::Instruction *I, DataId id = ProbeId);
  llvm::Value *getAddress() const;
  llvm::Type *getDataType() const;
  llvm::Value *getValue() const;
  void findVariableName();
  std::string getVariableName() const;
  void findDataTypeName();
  std::string getDataTypeName() const;  
  bool isValidDataType() const ;
  static inline bool classof(const Data *D) {
    return D->getValueId() == ProbeId;
  }
};

class PredicateData : public Data {
private:
  llvm::Value* condition;
  llvm::Value* ifBlock;
  llvm::Value* elseBlock;

public:
  PredicateData(llvm::Instruction *I, DataId id = PredicateId);
  llvm::Value *getCondition() const;
  llvm::Value *getAddress() const;
  void findDataTypeName();
  std::string getDataTypeName() const;
  bool isTemporaryVariable() const;
  void findVariableName();
  std::string getVariableName() const ;
  bool isValidOperation() const;
  llvm::Value* getValue() const;
  static inline bool classof(const Data *D) {
    return D->getValueId() == PredicateId;
  }
};

std::string getLocInfoStr(const Data &D);
  
Data *CreateData(llvm::Instruction *I);
}


#endif /* _DATA_DATA_HXX__ */
