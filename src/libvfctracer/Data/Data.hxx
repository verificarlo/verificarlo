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

#ifndef DATA_DATA_HXX
#define DATA_DATA_HXX

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Casting.h"
#include <string>

#include "../opcode.hxx"

namespace vfctracerData {

enum DataId { ScalarId, VectorId };

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

public:
  Data(llvm::Instruction *I, DataId Id);
  virtual ~Data() = 0;
  virtual llvm::Instruction *getData() const;
  virtual llvm::Value *getValue() const;
  virtual llvm::Type *getDataType() const;
  virtual llvm::Type *getDataPtrType() const;
  virtual bool isTemporaryVariable() const;
  virtual bool isValidOperation();
  virtual bool isValidDataType();
  virtual std::string getOriginalLine();
  virtual std::string getFunctionName();
  virtual std::string &getRawName();
  virtual std::string getDataTypeName() = 0;
  virtual llvm::Value *getAddress() const = 0;
  virtual std::string getVariableName() = 0;
  virtual void dump();
  DataId getValueId() const { return Id; };
};

class ScalarData : public Data {
public:
  ScalarData(llvm::Instruction *, DataId id = ScalarId);
  llvm::Value *getAddress() const;
  std::string getVariableName();
  std::string getDataTypeName();
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
  std::string getVariableName();
  std::string getDataTypeName();
  static inline bool classof(const Data *D) {
    return D->getValueId() == VectorId;
  }
};

Data *CreateData(llvm::Instruction *I);
}

#endif /* DATA_DATA_HXX */
