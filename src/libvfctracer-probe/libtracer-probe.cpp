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

#include "../libvfctracer/Data/Data.hxx"
#include "../libvfctracer/Format/Format.hxx"
#include "../libvfctracer/vfctracer.hxx"

#include <fstream>
#include <list>
#include <set>
#include <unordered_map>

using namespace llvm;

static cl::opt<bool> VfclibInstVerbose("vfclibinst-verbose",
                                       cl::desc("Activate verbose mode"),
                                       cl::value_desc("Verbose"),
                                       cl::init(false));

#if LLVM_VERSION_MAJOR == 4
auto binaryOpt = clEnumValN(vfctracerFormat::BinaryId, "binary", "Binary format");
auto textOpt = clEnumValN(vfctracerFormat::TextId, "text", "Text format");
auto formatValue = cl::values(binaryOpt, textOpt);
static cl::opt<vfctracerFormat::FormatId>
    VfclibFormat("vfclibtracer-format", cl::desc("Output format"),
                 cl::value_desc("TracerFormat"), formatValue);

auto basicOpt = clEnumValN(vfctracer::basic, "basic", "Basic level");
auto temporaryOpt = clEnumValN(vfctracer::temporary, "temporary",
                               "Tracing temporary variables");
auto levelOpt = cl::values(basicOpt, temporaryOpt);
static cl::opt<vfctracer::optTracingLevel>
    VfclibTracingLevel("vfclibtracer-level", cl::desc("Tracing Level"),
                       cl::value_desc("TracingLevel"), levelOpt);
#else
static cl::opt<vfctracerFormat::FormatId> VfclibFormat(
    "vfclibtracer-format", cl::desc("Output format"),
    cl::value_desc("TracerFormat"),
    cl::values(clEnumValN(vfctracerFormat::BinaryId, "binary", "Binary format"),
               clEnumValN(vfctracerFormat::TextId, "text", "Text format"),
               NULL) // sentinel
    );
static cl::opt<vfctracer::optTracingLevel> VfclibTracingLevel(
    "vfclibtracer-level", cl::desc("Tracing Level"),
    cl::value_desc("TracingLevel"),
    cl::values(clEnumValN(vfctracer::basic, "basic", "Basic level"),
               clEnumValN(vfctracer::temporary, "temporary",
                          "Allows to trace temporary variables"),
               NULL) // sentinel
    );
#endif

static cl::opt<bool> VfclibBacktrace("vfclibtracer-backtrace",
                                     cl::desc("Add backtrace function"),
                                     cl::value_desc("TracerBacktrace"),
                                     cl::init(false));

static cl::opt<bool> VfclibDebug("vfclibtracer-debug",
                                 cl::value_desc("TracerDebug"),
                                 cl::desc("Enable debug mode"),
                                 cl::init(false));

namespace {
  
struct VfclibTracerProbe : public ModulePass {
  static char ID;
  std::ofstream mappingFile;

  VfclibTracerProbe() : ModulePass(ID) {
    std::string mappingFilename(getenv("VERITRACER_LOCINFO_PATH"));
    mappingFilename += "/locationInfo.map";
    mappingFile.open(mappingFilename, std::fstream::out | std::fstream::app);
    if (not mappingFile.is_open()) {
      errs() << "Cannot open file : " << mappingFilename << "\n";
      exit(1);
    }
  }

  void raiseErrorMsg(vfctracerData::Data * D, const std::string &msg = "") {
    errs() << "Error while checking vfc_probe argument\n";
    if (D) D->dump(); else errs() << "nullptr\n";
    errs() << msg << "\n";
    exit(1);    
  }

  void raiseErrorMsg(Instruction * I, const std::string &msg = "") {
    errs() << "Error while checking vfc_probe argument\n";
    errs() << *I << "\n";
    errs() << msg << "\n";
    exit(1);    
  }
  
  bool insertBacktraceCall(Module *M, Function *F, Instruction *I,
                           vfctracerFormat::Format *Fmt,
                           vfctracerData::Data *D) {
    Type *voidTy = Type::getVoidTy(M->getContext());
    Type *locInfoType = Fmt->getLocInfoType(*D);
    Value *locInfoValue = Fmt->getOrCreateLocInfoValue(*D);
    std::string backtraceFunctionName = "get_backtrace";

    if (const vfctracerData::VectorData *VD =
            dyn_cast<vfctracerData::VectorData>(D))
      backtraceFunctionName += "_x" + std::to_string(VD->getVectorSize());

    Constant *hookFunc = M->getOrInsertFunction(backtraceFunctionName, voidTy,
                                                locInfoType, (Type *)0);

    IRBuilder<> builder(D->getData());
    /* For FP operations, need to insert the probe after the instruction */
    if (opcode::isFPOp(D->getData()))
      builder.SetInsertPoint(D->getData()->getNextNode());
    if (Function *fun = dyn_cast<Function>(hookFunc))
      builder.CreateCall(fun, locInfoValue, "");
    else
      llvm_unreachable("Hook function cannot be cast into function type");
    return true;
  }

  bool insertProbe(vfctracerFormat::Format &Fmt, vfctracerData::Data &D) {
    std::string variableName = D.getVariableName();
    if (VfclibDebug)
      D.dump(); /* /!\ Dump Data */
    if (not D.isValidDataType())
      raiseErrorMsg(&D, "is not a valid type or is a temporary variable");
    if (VfclibInstVerbose)
      vfctracer::VerboseMessage(D,"-probe");
    Constant *probeFunction = Fmt.CreateProbeFunctionPrototype(D);
    CallInst *probeCallInst = Fmt.InsertProbeFunctionCall(D, probeFunction);
    if (probeCallInst == nullptr) {
      errs() << "Error while instrumenting variable " + variableName;
      exit(1);
    }
    return true;
  }

  void eraseVfcProbeCall(std::vector<vfctracerData::Data*> &toErase) {
    for (auto &D : toErase)
      D->getData()->eraseFromParent();      
  }
  
  bool runOnBasicBlock(Module &M, BasicBlock &B, vfctracerFormat::Format &Fmt) {
    std::vector<vfctracerData::Data*> toErase;
    bool modified = false;
    for (Instruction &ii : B) {
      if (CallInst *callInst = dyn_cast<CallInst>(&ii))
	  if (opcode::isProbeOp(callInst)) {	    
	    vfctracerData::Data *D = vfctracerData::CreateData(callInst);
	    if (D == nullptr) continue;
	    if (not D->isValidOperation() || not D->isValidDataType()) {
	      raiseErrorMsg(D, "Data is not a valid operation or not a valid data type");
	    }
	    modified |= insertProbe(Fmt, *D);
	    if (modified) toErase.push_back(D);
	    if (VfclibBacktrace && modified){
#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR < 8
	      insertBacktraceCall(&M, ii.getParent()->getParent(), &ii, &Fmt, D);
#else
	      insertBacktraceCall(&M, ii.getFunction(), &ii, &Fmt, D);
#endif
	    }
	  }      
    }
    if (modified) eraseVfcProbeCall(toErase);      
    return modified;
  };
  
  bool runOnFunction(Module &M, Function &F, vfctracerFormat::Format &Fmt) {
    if (VfclibInstVerbose) {
      errs() << "In Function: ";
      errs().write_escaped(F.getName()) << '\n';
    }

    bool modified = false;

    for (Function::iterator bi = F.begin(), be = F.end(); bi != be; ++bi) {
      modified |= runOnBasicBlock(M, *bi, Fmt);
    }
    return modified;
  }

  bool runOnModule(Module &M) {
    bool modified = false;

    vfctracerFormat::Format *Fmt =
        vfctracerFormat::CreateFormat(M, VfclibFormat);
    
    std::vector<Function *> functions;
    for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
        functions.push_back(&*F);
    }

    // Finds functions with vfc_probe functions
    // and replaces them the veritracer_probe
    for (auto &F : functions) {
      modified |= runOnFunction(M, *F, *Fmt);
    }
    
    // Dump hash value
    vfctracer::dumpMapping(mappingFile);
    // runOnModule must return true if the pass modifies the IR
    return modified;
  }
};
}

char VfclibTracerProbe::ID = 0;
static RegisterPass<VfclibTracerProbe> X("vfclibtracer-probe", "veritracer probes pass", false,
                                    false);
