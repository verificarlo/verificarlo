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

#ifndef VERITRACER_HXX
#define VERITRACER_HXX

#include "llvm/IR/Type.h"

#include <fstream>
#include <unordered_map>
#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR <= 5 
#include "llvm/DebugInfo.h"
#include "llvm/Support/InstIterator.h"
#else
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/InstIterator.h"
#endif

#include "Data/Data.hxx"    
  
namespace vfctracer {

  enum optTracingLevel { basic = 0, temporary = 1 };
  extern vfctracer::optTracingLevel tracingLevel;
  
  typedef std::unordered_map<uint64_t, std::string> locinfomap;
  
  const std::string temporaryVariableName = "_";
  const std::string probePrefixName = "_veritracer_probe_";
  const std::string binarySuffixName = "_binary";
  const std::string floatTypeName = "binary32";
  const std::string doubleTypeName = "binary64";
  const std::string vectorName_x2 = "2x";
  const std::string vectorName_x4 = "4x";
  
  bool isValidDataType(llvm::Type *dataType);
  bool isValidOperation(vfctracerData::Data &D);
  std::string getLocInfo(vfctracerData::Data &D);
  uint64_t getOrInsertLocInfoValue(std::string &locInfo, std::string ext = "");
  std::string getBaseTypeName(llvm::Type *baseType);
  const llvm::Function *findEnclosingFunc(const llvm::Value *V);
  llvm::MDNode *findVar(const llvm::Value *V, const llvm::Function *F);
  std::string findName(const llvm::Value *V);
  void VerboseMessage(vfctracerData::Data &D);
  void dumpMapping(std::ofstream &mappingFile);
  std::string getRawName(const llvm::Value *V);
  std::string getRawName(const llvm::Instruction *I);
  void ltrim(std::string &s);
}

#endif /* VERITRACER_HXX */
