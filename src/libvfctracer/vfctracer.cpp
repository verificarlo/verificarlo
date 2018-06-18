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

#include "Data/Data.hxx"
#include "Format/Format.hxx"
#include "vfctracer.hxx"

using namespace llvm;
using namespace vfctracerData;
using namespace vfctracerFormat;
using namespace opcode;

namespace vfctracer {
  
  locinfomap locInfoMap = {};
  std::hash<std::string> locInfoHasher;
  optTracingLevel tracingLevel;
  
  bool isValidDataType(Type *dataType) {
    if (dataType->isFloatTy())
      return true;
    else if (dataType->isDoubleTy())
      return true;
    else
      return false;
  }

  bool isValidOperation(Data &D) {
    Instruction *I = D.getData();
    Fops opCode = getOpCode(I);
    /* Checks that the stored value is not a constant */
    if (opCode == Fops::STORE)
      return not isa<llvm::ConstantFP>(D.getValue());
    /* Checks that the returned value type is not void */
    if (opCode == Fops::RETURN)
      return not D.getDataType()->isVoidTy();
    return opCode != Fops::FOP_IGNORE;
  }

  uint64_t getOrInsertLocInfoValue(std::string &locInfo,
					      std::string ext) {
    std::string locInfoExt = locInfo + ext;
    uint64_t hashLocInfo = locInfoHasher(locInfoExt);
    locInfoMap[hashLocInfo] = locInfoExt;
    return hashLocInfo;
  }

  std::string getBaseTypeName(Type *baseType) {
    if (baseType->isFloatTy())
      return floatTypeName;
    else if (baseType->isDoubleTy())
      return doubleTypeName;
    else
      /* We must not be here 
       if isValidDataType
       correctly checks types */    
      llvm_unreachable("Wrong basetype");
  }

  const Function* findEnclosingFunc(const Value *V) {
    if (const Argument *Arg = dyn_cast<Argument>(V)) {
      return Arg->getParent();
    }
    if (const Instruction *I = dyn_cast<Instruction>(V)) {
      return I->getParent()->getParent();
    }
    return nullptr;
  }

  // renome into findDbgInstVar
  MDNode* findVar(const Value *V, const Function *F) {
    for (const_inst_iterator Iter = inst_begin(F), End = inst_end(F);
	 Iter != End; ++Iter) {
      const Instruction *I = &*Iter;
      if (const DbgValueInst *DbgValue = dyn_cast<DbgValueInst>(I)) {
	if (DbgValue->getValue() == V)
	  return DbgValue->getVariable();
      } else if (const DbgDeclareInst *DbgDeclare = dyn_cast<DbgDeclareInst>(I)) {
	if (DbgDeclare->getAddress() == V) {
	  errs() << "dbgdeclare " << *DbgDeclare << "\n";
	  return DbgDeclare->getVariable();
	}
      }
    }
    return nullptr;
  }

  /* The names of temporary expressions are constructed as */
  /* c = a op b */
  std::string buildTmpExprName(const Value *V) {
    if (const Instruction *I = dyn_cast<Instruction>(V)) {
      if (opcode::isFPOp(I)) {	
	Value * op0 = I->getOperand(0);
	Value * op1 = I->getOperand(1);
	std::string opStr = opcode::getOpStr(I);
	std::string nameop = vfctracer::getRawName(I);
	std::string name0 = vfctracer::getRawName(op0);
	std::string name1 = vfctracer::getRawName(op1);
	std::string name = nameop;
	name += " = ";
	name += name0;
	name += " " ;
	name += opStr ;
	name += " " ;
	name += name1 ;
	return name;
      } else if (opcode::isRetOp(I)) {
	return "ret " + vfctracer::getRawName(I->getOperand(0));
      }
    }
    return "";
  }  
  
  std::string findName(const Value *V) {
    const Function *F = findEnclosingFunc(V);
    if (F != nullptr) {
      std::string name = "";
#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR < 9      
      name = V->getName();
#else     
      MDNode *MD = findVar(V,F);
      if (MD != nullptr)
	if (DILocalVariable *DILoc = dyn_cast<DILocalVariable>(MD))
	  name = DILoc->getName();
#endif
      if (tracingLevel > optTracingLevel::basic)
	if (name.empty() || name == vfctracer::temporaryVariableName)
	  return buildTmpExprName(V);
      return name;      
    }
    
    return temporaryVariableName;
  }

  void VerboseMessage(Data &D) {
    errs() << "[Veritracer] Instrumenting" << *D.getData()
	   << " | Variable Name = " << D.getVariableName()
	   << " at " << D.getOriginalLine() << '\n';
  }

  std::string getLocInfo(Data &D) {
    std::string locInfo = D.getDataTypeName() + " " +
      D.getFunctionName() + " " +
      D.getOriginalLine() + " " +
      D.getVariableName();
    return locInfo;
  }

  /* Dump mapping information about variables  */
  /* hash : <line> <enclosing function> <name> */
  void dumpMapping(std::ofstream &mappingFile) {
    for(auto &I : locInfoMap) {
      mappingFile << I.first << ":" << I.second << "\n";
    }
  }

  void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
	  return !std::isspace(ch);
	}));
  }

  std::string getRawName(const Value *data) {
    /* Ugly hack to redirect a raw_fd_ostream (errs()) into a string */
    std::FILE * tmpF = std::tmpfile();
    int fd = fileno(tmpF);
    llvm::raw_fd_ostream f(fd,false,false);
    f << *data;
    uint64_t pos = f.tell();
    char *buf = (char*) malloc(sizeof(char) * f.GetBufferSize()); 
    std::rewind(tmpF);
    char *ret = std::fgets(buf, pos+1, tmpF);
    if (ret != buf) errs() << "Error while getting raw name\n";
    std::string tmp(buf);
    /* Split instruction and get first token which is the SSA form name */
    std::size_t pos_sep = tmp.find("=");
    std::string raw_name = tmp.substr(0,pos_sep);
    ltrim(raw_name);    
    return raw_name;
  }

  std::string getRawName(const Instruction *data) {
    /* Ugly hack to redirect a raw_fd_ostream (errs()) into a string */
    std::FILE * tmpF = std::tmpfile();
    int fd = fileno(tmpF);
    llvm::raw_fd_ostream f(fd,false,false);
    f << *data;
    uint64_t pos = f.tell();
    char *buf = (char*) malloc(sizeof(char) * f.GetBufferSize()); 
    std::rewind(tmpF);
    char *ret = std::fgets(buf, pos+1, tmpF);
    if (ret != buf) errs() << "Error while getting raw name\n";
    std::string tmp(buf);
    /* Split instruction and get first token which is the SSA form name */
    std::size_t pos_sep = tmp.find("=");
    std::string raw_name = tmp.substr(0,pos_sep);
    ltrim(raw_name);    
    return raw_name;
  }

}
