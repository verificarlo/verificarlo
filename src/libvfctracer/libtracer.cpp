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

#include "vfctracer.hxx"
#include "Data/Data.hxx"
#include "Format/Format.hxx"

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

using namespace llvm;
// VfclibInst pass command line arguments
static cl::opt<std::string> VfclibInstFunction("vfclibinst-function",
					       cl::desc("Only instrument given FunctionName"),
					       cl::value_desc("FunctionName"), cl::init(""));

static cl::opt<std::string> VfclibInstFunctionFile("vfclibinst-function-file",
						   cl::desc("Instrument functions in file FunctionNameFile "),
						   cl::value_desc("FunctionsNameFile"), cl::init(""));

static cl::opt<bool> VfclibInstVerbose("vfclibinst-verbose",
                                       cl::desc("Activate verbose mode"),
                                       cl::value_desc("Verbose"),
                                       cl::init(false));

static cl::opt<bool> VfclibBlackList("vfclibblack-list",
				     cl::desc("Activate black list mode"),
				     cl::value_desc("BlackList"), cl::init(false));

static cl::opt<bool> VfclibBinaryFormat("vfclibbinary-format",
					cl::desc("Output information in binary format"),
					cl::value_desc("BinaryFormat"), cl::init(true));

static cl::opt<bool> VfclibBacktrace("vfclibbacktrace",
				     cl::desc("Add backtrace function"),
				     cl::value_desc("Backtrace"), cl::init(false));

namespace {
  
  // Define an enum type to classify the floating points operations
  // that are instrumented by verificarlo

  enum Fops {FOP_ADD, FOP_SUB, FOP_MUL, FOP_DIV, STORE, FOP_IGNORE};

  // Each instruction can be translated to a string representation

  std::string Fops2str[] = { "add", "sub", "mul", "div", "store", "ignore"};

  const std::string tmpVarName = "_";
  const std::string locationInfoStr = "locationInfo.str";
  
  struct VfclibTracer : public ModulePass {
    static char ID;
    StructType *mca_interface_type;
    std::set<std::string> SelectedFunctionSet;
    std::unordered_map<uint64_t, std::string> locationInfoMap;
    std::unordered_map<uint64_t, std::string> locInfoMap;
    std::hash<std::string> hashStrFunction;
    std::ofstream mappingFile;
  
    VfclibTracer() : ModulePass(ID) {
      if (not VfclibInstFunctionFile.empty()) {
	std::string line;
	std::ifstream loopstream (VfclibInstFunctionFile.c_str());
	if (loopstream.is_open()) {
	  while (std::getline(loopstream, line)) {
	    SelectedFunctionSet.insert(line);
	  }
	  loopstream.close();
	} else {
	  errs() << "Cannot open " << VfclibInstFunctionFile << "\n";
	  assert(0);
	}
      } else if (not VfclibInstFunction.empty()) {
	SelectedFunctionSet.insert(VfclibInstFunction);
      }
      if (VfclibBinaryFormat) {
	std::string mappingFilename(getenv("VERITRACER_LOCINFO_PATH"));
	mappingFilename += "/locationInfo.map";
	mappingFile.open(mappingFilename,  std::fstream::out | std::fstream::app);
	if (not mappingFile.is_open()) {
	  errs() << "Cannot open file : "  << mappingFilename << "\n";
	  exit(1);
	} else {
	  // errs() << mappingFilename << " is open \n";
	}
      }
    }    

    void getInfoMD(const MDNode *v) {
      if (v != nullptr) {
	DILocation DILoc(v);
	unsigned line = DILoc.getLineNumber() ;
	unsigned col = DILoc.getColumnNumber() ;
	// DIScope scope = DILoc.getScope() ;
	// DILocation origloc = DILoc.getOrigLocation() ; 

	DIVariable var(v);

	// scope = var.getContext();
	StringRef name = var.getName();
	// DIFile file = var.getFile();
	line = var.getLineNumber();    
	// unsigned args =  var.getArgNumber();
	// DITypeRef type = var.getType();
      }
    }

    const Function *findEnclosingFunc(const Value *V) {
      if (const Argument *Arg = dyn_cast<Argument>(V)) {
	return Arg->getParent();
      }
      if (const Instruction *I = dyn_cast<Instruction>(V)) {
	return I->getParent()->getParent();
      }
      return NULL;
    }

    const MDNode *findVar(const Value *V, const Function *F) {
      for (const_inst_iterator Iter = inst_begin(F), End = inst_end(F);
	   Iter != End; ++Iter) {
	const Instruction *I = &*Iter;
	if (const DbgValueInst *DbgValue = dyn_cast<DbgValueInst>(I)) {
	  if (DbgValue->getValue() == V)
	    return DbgValue->getVariable();
	} else if (const DbgDeclareInst *DbgDeclare = dyn_cast<DbgDeclareInst>(I)) {
	  if (DbgDeclare->getAddress() == V)
	    return DbgDeclare->getVariable();
	} 
      }
      return NULL;
    }
        
    StringRef getOriginalName(const Value *V) {      

      if (const Instruction *I = dyn_cast<Instruction>(V)) {
	if (I->isBinaryOp() && I->getType()->isVectorTy()) {

	  Value *op0 = I->getOperand(0); 
	  Value *op1 = I->getOperand(1); 

	  StringRef name_op0 = "";
	  StringRef name_op1 = "";
	  
	  // <result> = load [volatile] <ty>, <ty>* <pointer>
	  if (const LoadInst *Load = dyn_cast<LoadInst>(op0)) {
	    Value * ptrOp = Load->getOperand(0);
	    // <result> = bitcast <ty> <value> to <ty2>             ; yields ty2
	    if (const BitCastInst *Bitcast = dyn_cast<BitCastInst>(ptrOp)) {
	      Value * V = Bitcast->getOperand(0);
	      Value * ty2 = Bitcast->getOperand(1);
	      return getOriginalName(V);
	    }
	  }
	
	  // <result> = load [volatile] <ty>, <ty>* <pointer>
	  if (const LoadInst *Load = dyn_cast<LoadInst>(op1)) {
	    Value * ptrOp = Load->getOperand(0);
	    // <result> = bitcast <ty> <value> to <ty2>             ; yields ty2
	    if (const BitCastInst *Bitcast = dyn_cast<BitCastInst>(ptrOp)) {
	      Value * V = Bitcast->getOperand(0);
	      Value * ty2 = Bitcast->getOperand(1);
	      return getOriginalName(V);
	    }
	  }

	}	
      }
      
      
      // If the value is defined as a GetElementPtrInstruction, return the name
      // of the pointer operand instead
      if (const GetElementPtrInst *I = dyn_cast<GetElementPtrInst>(V)) {
	return getOriginalName(I->getPointerOperand());
      }
      // If the value is a constant Expr, such as a GetElementPtrInstConstantExpr,
      // try to get the name of its first operand
      if (const ConstantExpr *E = dyn_cast<ConstantExpr>(V)) {
	return getOriginalName(E->getOperand(0));
      }

      if (const ConstantVector *CV = dyn_cast<ConstantVector>(V)) {
	return getOriginalName(CV->getOperand(0));
      }

      if (const Instruction *I = dyn_cast<Instruction>(V)) {
	if (isFPop(*I)) {
	  Value *v0 = I->getOperand(0);
	  Value *v1 = I->getOperand(1);
	  
	  StringRef name0 = findName(v0);
	  StringRef name1 = findName(v1);
	  
	  if (name0 != "_")
	    return name0;
	  if (name1 != "_")
	    return name1;	    		
	}
      }      
      return findName(V);
    }

    StringRef findName(const Value *V){
      const Function *F = findEnclosingFunc(V);
      if (!F)
	return V->getName();

      const MDNode *Var = findVar(V, F);
      if (!Var)
	return tmpVarName;

      StringRef name = DIVariable(Var).getName();
      return name;
    }
    
    // Use std::string instead of StringRef
    // Sources: http://llvm.org/docs/ProgrammersManual.html#dss-twine
    //
    // "2.For the same reason, StringRef cannot be used as the return value
    //   of a method if the method “computes” the result string. Instead, use std::string."
    std::string getOriginalLine(const Instruction &I) {
      std::string str_to_return = "_ _";
      if (MDNode *N = I.getMetadata(LLVMContext::MD_dbg)) {
	DILocation Loc(N);
	do {
	  std::string Line = std::to_string(Loc.getLineNumber());
	  std::string File = Loc.getFilename();
	  std::string Dir = Loc.getDirectory();
	  DILocation Loc = Loc.getOrigLocation(); 
	  str_to_return = File + " " + Line;     
	} while(str_to_return == "_ _" && Loc != nullptr);
      }
      return str_to_return;
    }

    bool insertBacktraceCall(Module *M, Function *F, Instruction *I,
			     vfctracerFormat::Format *Fmt, vfctracerData::Data *D) {
      Type *voidTy = Type::getVoidTy(M->getContext());
      Type *locInfoType = Fmt->getLocInfoType(*D);
      Value *locInfoValue = Fmt->getOrCreateLocInfoValue(*D);
      std::string backtraceFunctionName = "get_backtrace";

      if (vfctracerData::VectorData* VD = dynamic_cast<vfctracerData::VectorData*>(D))
	backtraceFunctionName += "_x" + std::to_string(VD->getVectorSize());
      
      Constant *hookFunc = M->getOrInsertFunction(backtraceFunctionName,
						  voidTy,
						  locInfoType,
						  (Type *)0);

      
      IRBuilder<> builder(D->getData());
      builder.SetInsertPoint(D->getData()->getNextNode());
      builder.CreateCall(cast<Function>(hookFunc),locInfoValue,"");
      return true;
    }

    // Debug function
    // Print all dbg.intrinsic of function F
    void printDbgInstrinsic(Function &F) {
      for (inst_iterator Iter = inst_begin(F), End = inst_end(F);
      	   Iter != End; ++Iter) {
      	const Instruction *I = &*Iter;
      	MDNode *v = nullptr;	
      	if (const DbgValueInst *DbgValue = dyn_cast<DbgValueInst>(I)) {
      	  v = DbgValue->getVariable();
	  errs() << *DbgValue << "\n";
      	} else if (const DbgDeclareInst *DbgDeclare = dyn_cast<DbgDeclareInst>(I)) {
	  v = DbgDeclare->getVariable();
	  errs() << *DbgDeclare << "\n";
  	}
      }
    }
    
    Fops mustReplace(Instruction &I) {
      switch (I.getOpcode()) {
      case Instruction::FAdd:
	return FOP_ADD;
      case Instruction::FSub:
	// In LLVM IR the FSub instruction is used to represent FNeg
	return FOP_SUB;
      case Instruction::FMul:
	return FOP_MUL;
      case Instruction::FDiv:
	return FOP_DIV;
      case Instruction::Store:
	return STORE;
	// case Instruction::Ret:
	// 	return RETURN;
      default:
	return FOP_IGNORE;
      }
    }

    bool isFPop(const Instruction &I) {
      switch (I.getOpcode()) {
      case Instruction::FAdd:
      case Instruction::FSub:
      case Instruction::FMul:
      case Instruction::FDiv:
	return true;
      default:
	return false;
      }
    }
    
    bool insertProbe(vfctracerFormat::Format &Fmt, vfctracerData::Data &D) {
      Type *dataType = D.getDataType();
      Type *ptrDataType = D.getDataPtrType();
      std::string variableName = D.getVariableName();
      // D.dump();
      if (not D.isValidDataType() || D.isTemporyVariable())
	return false;
      if (VfclibInstVerbose)
	vfctracer::VerboseMessage(D);      
      Constant *probeFunction = Fmt.CreateProbeFunctionPrototype(D);
      CallInst *probeCallInst = Fmt.InsertProbeFunctionCall(D, probeFunction);
      return true;
    }

    bool runOnBasicBlock(Module &M, BasicBlock &B, vfctracerFormat::Format &Fmt) {
      bool modified = false;
      for (BasicBlock::iterator ii = B.begin(), ie = B.end(); ii != ie; ++ii) {
	vfctracerData::Data *D = vfctracerData::CreateData(ii);
	if (not D->isValidOperation() || not D->isValidDataType()) continue;
	insertProbe(Fmt, *D);
	if (VfclibBacktrace) insertBacktraceCall(&M, ii->getParent()->getParent(), ii, &Fmt, D);      
      }
      return modified;
    };

    bool runOnFunction(Module &M, Function &F, vfctracerFormat::Format &Fmt) {
      if (VfclibInstVerbose) {
	errs() << "In Function: ";
	errs().write_escaped(F.getName()) << '\n';
      }
      // printDbgInstrinsic(F);

      bool modified = false;
    
      for (Function::iterator bi = F.begin(), be = F.end(); bi != be; ++bi) {
	modified |= runOnBasicBlock(M, *bi, Fmt);
      }
    
      return modified;
    }

    bool runOnModule(Module &M) {
      bool modified = false;

      StringRef SelectedFunction = StringRef(VfclibInstFunction);

      vfctracerFormat::Format* Fmt = vfctracerFormat::CreateFormat(M, VfclibBinaryFormat);
    
      // Find the list of functions to instrument
      // Instrumentation adds stubs to mcalib function which we
      // never want to instrument.  Therefore it is important to
      // first find all the functions of interest before
      // starting instrumentation.

      std::vector<Function *> functions;
      for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
	const bool is_in = SelectedFunctionSet.find(F->getName()) != SelectedFunctionSet.end();
	if (SelectedFunctionSet.empty() || VfclibBlackList != is_in) {
	  functions.push_back(&*F);
	} 
      }
      // Do the instrumentation on selected functions
      for (std::vector<Function *>::iterator F = functions.begin(); F != functions.end(); ++F) {
	modified |= runOnFunction(M, **F, *Fmt);
      }
      // Dump hash value 
      vfctracer::dumpMapping(mappingFile);
      // runOnModule must return true if the pass modifies the IR
      return modified;
    }
  };
}

char VfclibTracer::ID = 0;
static RegisterPass<VfclibTracer> X("vfclibtracer", "veritracer pass",
				    false, false);
