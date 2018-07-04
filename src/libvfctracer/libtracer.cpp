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

#include "Data/Data.hxx"
#include "Format/Format.hxx"
#include "vfctracer.hxx"

#include <fstream>
#include <list>
#include <set>
#include <unordered_map>

#if LLVM_VERSION_MINOR <= 6
#define CREATE_CALL2(func, op1, op2) (builder.CreateCall2(func, op1, op2, ""))
#define CREATE_STRUCT_GEP(i, p) (builder.CreateStructGEP(i, p))
#else
#define CREATE_CALL2(func, op1, op2) (builder.CreateCall(func, {op1, op2}, ""))
#define CREATE_STRUCT_GEP(i, p) (builder.CreateStructGEP(nullptr, i, p, ""))
#endif

using namespace llvm;

// VfclibInst pass command line arguments
static cl::opt<std::string>
    VfclibInstFunction("vfclibinst-function",
                       cl::desc("Only instrument given FunctionName"),
                       cl::value_desc("FunctionName"), cl::init(""));

static cl::opt<std::string> VfclibInstFunctionFile(
    "vfclibinst-function-file",
    cl::desc("Instrument functions in file FunctionNameFile "),
    cl::value_desc("FunctionsNameFile"), cl::init(""));

static cl::opt<bool> VfclibInstVerbose("vfclibinst-verbose",
                                       cl::desc("Activate verbose mode"),
                                       cl::value_desc("Verbose"),
                                       cl::init(false));

static cl::opt<bool> VfclibBlackList("vfclibblack-list",
                                     cl::desc("Activate black list mode"),
                                     cl::value_desc("BlackList"),
                                     cl::init(false));

#if LLVM_VERSION_MAJOR == 4
auto binaryOpt = clEnumValN(vfctracerFormat::BinaryId, "binary", "Binary format");
auto textOpt = clEnumValN(vfctracerFormat::TextId, "text", "Text format");
auto formatValue = cl::values(binaryOpt, textOpt);
static cl::opt<vfctracerFormat::optFormat>
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
static cl::opt<vfctracerFormat::optFormat> VfclibFormat(
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

// Define an enum type to classify the floating points operations
// that are instrumented by verificarlo

enum Fops { FOP_ADD, FOP_SUB, FOP_MUL, FOP_DIV, STORE, FOP_IGNORE };

// Each instruction can be translated to a string representation

std::string Fops2str[] = {"add", "sub", "mul", "div", "store", "ignore"};

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
      std::ifstream loopstream(VfclibInstFunctionFile.c_str());
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
    std::string mappingFilename(getenv("VERITRACER_LOCINFO_PATH"));
    mappingFilename += "/locationInfo.map";
    mappingFile.open(mappingFilename, std::fstream::out | std::fstream::app);
    if (not mappingFile.is_open()) {
      errs() << "Cannot open file : " << mappingFilename << "\n";
      exit(1);
    }
    vfctracer::tracingLevel = VfclibTracingLevel;
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
      builder.CreateCall(cast<Function>(fun), locInfoValue, "");
    else
      llvm_unreachable("Hook function cannot be cast into function type");
    return true;
  }

  // Debug function
  // Print all dbg.intrinsic of function F
  void printDbgInstrinsic(Function &F) {
    for (inst_iterator Iter = inst_begin(F), End = inst_end(F); Iter != End;
         ++Iter) {
      const Instruction *I = &*Iter;
      if (const DbgValueInst *DbgValue = dyn_cast<DbgValueInst>(I)) {
        errs() << *DbgValue << "\n";
      } else if (const DbgDeclareInst *DbgDeclare =
                     dyn_cast<DbgDeclareInst>(I)) {
        errs() << *DbgDeclare << "\n";
      }
    }
  }

  bool insertProbe(vfctracerFormat::Format &Fmt, vfctracerData::Data &D) {
    std::string variableName = D.getVariableName();
    if (VfclibDebug)
      D.dump(); /* /!\ Dump Data */
    if (not D.isValidDataType() || D.isTemporaryVariable())
      return false;
    if (VfclibInstVerbose)
      vfctracer::VerboseMessage(D);
    Constant *probeFunction = Fmt.CreateProbeFunctionPrototype(D);
    CallInst *probeCallInst = Fmt.InsertProbeFunctionCall(D, probeFunction);
    if (probeCallInst == nullptr) {
      errs() << "Error while instrumenting variable " + variableName;
      exit(1);
    }
    return true;
  }

  bool runOnBasicBlock(Module &M, BasicBlock &B, vfctracerFormat::Format &Fmt) {
    bool modified = false;
    for (Instruction &ii : B) {
      vfctracerData::Data *D = vfctracerData::CreateData(&ii);
      if (D == nullptr || not D->isValidOperation() || not D->isValidDataType())
        continue;
      modified |= insertProbe(Fmt, *D);
      if (VfclibBacktrace && modified)
#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR < 8
        insertBacktraceCall(&M, ii.getParent()->getParent(), &ii, &Fmt, D);
#else
        insertBacktraceCall(&M, ii.getFunction(), &ii, &Fmt, D);
#endif
    }
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

    // Find the list of functions to instrument
    // Instrumentation adds stubs to mcalib function which we
    // never want to instrument.  Therefore it is important to
    // first find all the functions of interest before
    // starting instrumentation.

    std::vector<Function *> functions;
    for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
      const bool is_in =
          SelectedFunctionSet.find(F->getName()) != SelectedFunctionSet.end();
      if (SelectedFunctionSet.empty() || VfclibBlackList != is_in) {
        functions.push_back(&*F);
      }
    }
    // Do the instrumentation on selected functions
    for (std::vector<Function *>::iterator F = functions.begin();
         F != functions.end(); ++F) {
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
static RegisterPass<VfclibTracer> X("vfclibtracer", "veritracer pass", false,
                                    false);
