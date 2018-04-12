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

#ifndef FORMAT_FORMAT_HXX
#define FORMAT_FORMAT_HXX

#include <string>
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"

#include "../Data/Data.hxx"

namespace vfctracerFormat {
  
  class Format {
  protected:
    llvm::Module* M;
    llvm::Type* locInfoType;
    llvm::Value* locInfoValue;
  public:
    virtual llvm::Type* getLocInfoType(vfctracerData::Data& D) = 0;
    virtual llvm::Constant* CreateProbeFunctionPrototype(vfctracerData::Data& D) = 0;
    virtual llvm::Value* getOrCreateLocInfoValue(vfctracerData::Data& D) = 0;
    virtual llvm::CallInst* InsertProbeFunctionCall(vfctracerData::Data& D, llvm::Value *probeFunc) = 0;
  };

  class BinaryFmt : public Format {
  public:
    BinaryFmt(llvm::Module &M);
    llvm::Type* getLocInfoType(vfctracerData::Data &D);
    llvm::Value* getOrCreateLocInfoValue(vfctracerData::Data &D);
    llvm::Constant* CreateProbeFunctionPrototype(vfctracerData::Data &D);      
    llvm::CallInst* InsertProbeFunctionCall(vfctracerData::Data &D, llvm::Value *probeFunc);
  };

  class TextFmt : public Format {
  public:
    TextFmt(llvm::Module &M);
    llvm::Type* getLocInfoType(vfctracerData::Data &D);
    llvm::Value* getOrCreateLocInfoValue(vfctracerData::Data &D);    
    llvm::Constant* CreateProbeFunctionPrototype(vfctracerData::Data &D);
    llvm::CallInst* InsertProbeFunctionCall(vfctracerData::Data &D, llvm::Value *probeFunc);
  };

  enum optFormat {
    binary,
    text
  };
  
  Format* CreateFormat(llvm::Module &M, vfctracerFormat::optFormat optFmt);

}

#endif /* FORMAT_FORMAT_HXX */
