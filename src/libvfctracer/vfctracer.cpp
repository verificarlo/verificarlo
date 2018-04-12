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
  };

  bool isValidOperation(Data &D) {
    Instruction *I = D.getData();
    Fops opCode = getOpCode(I);
    /* Check value stored is not a constant */
    if (opCode == Fops::STORE)
      return not isa<llvm::ConstantFP>(D.getValue());
    return opCode != Fops::FOP_IGNORE;
  };

  uint64_t getOrInsertLocInfoValue(std::string &locInfo,
					      std::string ext) {
    std::string locInfoExt = locInfo + ext;
    uint64_t hashLocInfo = locInfoHasher(locInfoExt);
    locInfoMap[hashLocInfo] = locInfoExt;
    return hashLocInfo;
  };

  std::string getBaseTypeName(Type *baseType) {
    if (baseType->isFloatTy())
      return floatTypeName;
    else if (baseType->isDoubleTy())
      return doubleTypeName;
    else
      llvm_unreachable("Wrong basetype");
      /* We must not be here 
       if isValidDataType
       correctly checks types */    
  };

  const Function* findEnclosingFunc(const Value *V) {
    if (const Argument *Arg = dyn_cast<Argument>(V)) {
      return Arg->getParent();
    }
    if (const Instruction *I = dyn_cast<Instruction>(V)) {
      return I->getParent()->getParent();
    }
    return nullptr;
  };

  std::set<const MDNode*> findVars(const Value *V, const Function *F) {
    std::set<const MDNode*> set;
    if (const GlobalValue *gv = dyn_cast<GlobalValue>(V))
      errs() << "GVal\n";
    if (const GlobalVariable *gv = dyn_cast<GlobalVariable>(V))
      errs() << "GVar\n";
    errs() << *V << "&" << F << "\n";
    if (F == nullptr)
      return set;
    for (const_inst_iterator Iter = inst_begin(F), End = inst_end(F);
	 Iter != End; ++Iter) {
      const Instruction *I = &*Iter;
      if (const DbgValueInst *DbgValue = dyn_cast<DbgValueInst>(I)) {
	errs() << *DbgValue << "\n";
	if (DbgValue->getValue() == V)
	  set.insert(DbgValue->getVariable());
      } else if (const DbgDeclareInst *DbgDeclare = dyn_cast<DbgDeclareInst>(I)) {
	errs() << *DbgDeclare << "\n";
	if (DbgDeclare->getAddress() == V)
	  set.insert(DbgDeclare->getVariable());
      }
    }
    return set;
  }
  
  const MDNode* findVar(const Value *V, const Function *F) {
    for (const_inst_iterator Iter = inst_begin(F), End = inst_end(F);
	 Iter != End; ++Iter) {
      const Instruction *I = &*Iter;
      if (const DbgValueInst *DbgValue = dyn_cast<DbgValueInst>(I)) {
	if (DbgValue->getValue() == V)
	  return DbgValue->getVariable();
      } else if (const DbgDeclareInst *DbgDeclare = dyn_cast<DbgDeclareInst>(I)) {
	MDNode *var = DbgDeclare->getVariable();
	if (DbgDeclare->getAddress() == V)
	  return DbgDeclare->getVariable();
      } 
    }
    return nullptr;
  };
  
  /* The names of temporary expressions are constructed as */
  /* c = a op b */
  std::string buildTmpExpr(const Value *V) {
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
  };  
  
  std::string findName(const Value *V) {
    /* Change boolean */
    bool followTemporaryVariable = true;
    const Function *F = findEnclosingFunc(V);
    if (F != nullptr) {
      std::string name = V->getName();
      if (tracingLevel > optTracingLevel::basic)
	if (name.empty() || name == vfctracer::temporaryVariableName)
	  return buildTmpExpr(V);
      return name;      
    }
    
    std::set<const MDNode*> vars = findVars(V, F);
    if (tracingLevel > optTracingLevel::basic) {
      std::string tmp = vfctracer::getRawName(V);
      return tmp;
    }
    
    if (vars.empty())
      return temporaryVariableName;
    
    errs() << "Error in find name\n";
    std::string first = DIVariable(*vars.begin()).getName().str();
    std::string name = std::accumulate(std::next(vars.begin()),
				       vars.end(),
				       first,
				       [](std::string a, const MDNode *b)
				       { return a + "," + DIVariable(b).getName().str(); });
    errs() << name << "\n";
    return name;
  };

  void VerboseMessage(Data &D) {
    errs() << "[Veritracer] Instrumenting" << *D.getData()
	   << " | Variable Name = " << D.getVariableName()
	   << " at " << D.getOriginalLine() << '\n';
  };

  std::string getLocInfo(Data &D) {
    std::string locInfo = D.getOriginalLine() + " " +
      D.getFunctionName() + " " +
      D.getVariableName() + " " +
      D.getDataTypeName();
    return locInfo;
  };

  /* Dump mapping information about variables  */
  /* hash : <line> <enclosing function> <name> */
  void dumpMapping(std::ofstream &mappingFile) {
    for(auto &I : locInfoMap) {
      mappingFile << I.first << ":" << I.second << "\n";
    }
  };

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
    char buf[f.GetBufferSize()];
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
    char buf[f.GetBufferSize()];
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

 void printInfoDIDescriptor(DIDescriptor *DI) {
    errs() << "Descriptor is " ;
    if (DI->isDerivedType()) errs() << "derivedType\n";
    else if (DI->isCompositeType()) errs() << "CompositeType\n";
    else if (DI->isBasicType()) errs() << "BasicType\n";
    else if (DI->isVariable()) errs() << "Variable\n";
    else if (DI->isSubprogram()) errs() << "Subprogram\n";
    else if (DI->isGlobalVariable()) errs() << "GlobalVariable\n";
    else if (DI->isScope()) errs() << "Scope\n";
    else if (DI->isFile()) errs() << "File\n";
    else if (DI->isCompileUnit()) errs() << "CompileUnit\n";
    else if (DI->isNameSpace()) errs() << "NameSpace\n";
    else if (DI->isLexicalBlockFile()) errs() << "LexicalBlockFile\n";
    else if (DI->isLexicalBlock()) errs() << "LexicalBlock\n";
    else if (DI->isSubrange()) errs() << "Subrange\n";
    else if (DI->isEnumerator()) errs() << "Enumerator\n";
    else if (DI->isType()) errs() << "Type\n";
    else if (DI->isUnspecifiedParameter()) errs() << "UnspecifiedParameter\n";
    else if (DI->isTemplateTypeParameter()) errs() << "TemplateTypeParameter\n";
    else if (DI->isTemplateValueParameter()) errs() << "TemplateValueParameter\n";
    else if (DI->isObjCProperty()) errs() << "ObjCProperty\n";
    else if (DI->isImportedEntity()) errs() << "ImportedEntity\n";
    else errs() << "unknown !\n";
  }

 void printInfoDIDescriptor(Value *v) {
    if (const MDNode *N = dyn_cast<MDNode>(v)) {
      DIDescriptor DI(N);
      printInfoDIDescriptor(DI);
    }
  }
}
