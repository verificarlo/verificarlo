/********************************************************************************
 *                                                                              *
 *  This file is part of Verificarlo.                                           *
 *                                                                              *
 *  Copyright (c) 2015                                                          *
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

#include "../../config.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/TypeBuilder.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <set>
#include <fstream>

const std::string LVL_MODULE = "module";
const std::string LVL_FUNCTION = "function";
const std::string  LVL_BASICBLOCK = "basicblock";
using namespace llvm;

static cl::opt<std::string> VfclibReportLevel("vfclibreport-level",
				      cl::desc("Report level"),
				      cl::value_desc("Level"),
				      cl::init(LVL_FUNCTION));

static cl::opt<bool> VfclibReportPrintPercentage("vfclibreport-percentage",
						 cl::desc("Print FPOps percentage"),
						 cl::value_desc("FPOps%%"),
						 cl::init(false));

namespace {
  // Define an enum type to classify the floating points operations
  // that are instrumented by verificarlo
  enum Fops {FOP_ADD, FOP_SUB, FOP_MUL, FOP_DIV, FOP_IGNORE};
    
  const std::string reportFilename = "verificarlo_report.csv"; 
  
  void operator+=(std::map<Fops, int> & dest, std::map<Fops, int> & src) {
    for (std::map<Fops,int>::iterator it = src.begin(), ie = src.end(); it != ie; ++it) {
      dest[it->first] += src[it->first];
    }
  };

  struct VfclibReport : public ModulePass {
    static char ID;
    std::map<std::string, int> ReportFPOpsMap;
    std::ofstream reportFile;
    std::map<std::string, std::map<Fops, int> > FPOpsTypeMap;
    VfclibReport() : ModulePass(ID) {   
      char *reportFilename = getenv("VERIFICARLO_REPORT_PATH");
      reportFile.open(reportFilename, std::fstream::out | std::fstream::app);
    }

    void printPercentage(const std::string &name) {
      if (not VfclibReportPrintPercentage)
	return;

      std::map<Fops, int> fopsMap = FPOpsTypeMap[name];
      int nbTotalFops = ReportFPOpsMap[name];
      nbTotalFops = (nbTotalFops != 0) ? nbTotalFops : 1;
      int nbAdd = fopsMap[FOP_ADD];
      int nbSub = fopsMap[FOP_SUB];
      int nbMul = fopsMap[FOP_MUL];
      int nbDiv = fopsMap[FOP_DIV];
      
      reportFile << "," << float(nbAdd)/nbTotalFops
		 << "," << float(nbSub)/nbTotalFops
		 << "," << float(nbMul)/nbTotalFops
		 << "," << float(nbDiv)/nbTotalFops;
    }
    
    void writeLineReport(Module &M) {
      // M.getName() not available with llvm-3.5
      const std::string moduleName = M.getModuleIdentifier();
      if (VfclibReportLevel == LVL_MODULE) {
	reportFile << moduleName << ","
		   << ReportFPOpsMap[moduleName];	  
	printPercentage(moduleName);
	reportFile << "\n";
      }
    }

    void writeLineReport(Module &M, Function &F) {
      const std::string moduleName = M.getModuleIdentifier();
      const std::string functionName = F.getName().str();
      if (VfclibReportLevel == LVL_FUNCTION) {
	reportFile << moduleName << ","
		   << functionName << ","
		   << ReportFPOpsMap[functionName];	  
	printPercentage(functionName);
	reportFile << "\n";
      }
    }
    
    void writeLineReport(Module &M, Function &F, BasicBlock &BB) {
      const std::string moduleName = M.getModuleIdentifier();
      const std::string functionName = F.getName().str();
      const std::string basicBlockName = BB.getName().str();
      if (VfclibReportLevel == LVL_BASICBLOCK) {
	reportFile << moduleName << ","
		   << functionName << ","
		   << basicBlockName << ","
		   << ReportFPOpsMap[basicBlockName];
	printPercentage(basicBlockName);
	reportFile << "\n";
      }
    }
    
    void printReport(Module &M) {	  
      writeLineReport(M);
      for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
	  writeLineReport(M, *F);
	  for (Function::iterator BB = F->begin(), BE = F->end(); BB != BE; ++BB) {
	    writeLineReport(M, *F, *BB);
	  }
      }	
    }     
   
    // Ftype getOpType(Instruction &I) {
    //   Type * retType = I.getType();
    //   Type * opType = I.getOperand(0)->getType();
    //   std::string opName = Fops2str[opCode];
	    
    //   std::string baseTypeName = "";
    //   std::string vectorName = "";
    //   Type *baseType = opType;

    //   // Check for vector types
    //   if (opType->isVectorTy()) {
    // 	VectorType *t = static_cast<VectorType *>(opType);
    // 	baseType = t->getElementType();
    // 	unsigned size = t->getNumElements();
	
    // 	if (size == 2) {
    // 	  vectorName = "2x";
    // 	} else if (size == 4) {
    // 	  vectorName = "4x";
    // 	} else {
    // 	  errs() << "Unsuported vector size: " << size << "\n";
    // 	  assert(0);
    // 	}
    //   }
      
    //   // Check the type of the operation
    //   if (baseType->isDoubleTy()) {
    // 	baseTypeName = "double";
    //   } else if (baseType->isFloatTy()) {
    // 	baseTypeName = "float";
    //   } else {
    // 	errs() << "Unsupported operand type: " << *opType << "\n";
    // 	assert(0);
    //   }
    // }
    
    Fops getOpCode(Instruction &I) {
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
      default:
	return FOP_IGNORE;
      }
    }
    
    bool runOnModule(Module &M) {
      int nbFPOps = 0;
      std::vector<Function*> functions;
      std::string moduleName = M.getModuleIdentifier();
      std::string functionName;
      FPOpsTypeMap[moduleName] = {};
      for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
	functionName = F->getName();
	runOnFunction(M, *F);
	nbFPOps += ReportFPOpsMap[functionName];
	FPOpsTypeMap[moduleName] += FPOpsTypeMap[functionName];
      }
      ReportFPOpsMap[moduleName] = nbFPOps;
      printReport(M);
      return false;      
    }
    
    bool runOnFunction(Module &M, Function &F) {
      int nbFPOps = 0;
      std::string functionName = F.getName();
      std::string basicBlockName;
      for (Function::iterator BB = F.begin(), BE = F.end(); BB != BE; ++BB) {
	runOnBasicBlock(M, *BB);
	basicBlockName = BB->getName();
	nbFPOps += ReportFPOpsMap[basicBlockName];
	FPOpsTypeMap[functionName] += FPOpsTypeMap[basicBlockName];
      }
      ReportFPOpsMap[functionName] = nbFPOps;
      return false;
    }

    bool runOnBasicBlock(Module &M, BasicBlock &BB) {
      int nbFPOps = 0;
      std::string basicBlockName = BB.getName();
      for (BasicBlock::iterator I = BB.begin(), IE = BB.end(); I != IE; ++I) {
	Fops fops = getOpCode(*I);
	if (fops != FOP_IGNORE) {
	  FPOpsTypeMap[basicBlockName][fops]++;
	  nbFPOps++;
	}
      }
      ReportFPOpsMap[basicBlockName] = nbFPOps;
      return false;
    }
  };
  

}

char VfclibReport::ID = 0;
static RegisterPass<VfclibReport> X("vfclibreport", "report pass", false, false);


