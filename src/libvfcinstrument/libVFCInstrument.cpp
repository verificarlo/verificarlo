/******************************************************************************
 *                                                                            *
 *  This file is part of Verificarlo.                                         *
 *                                                                            *
 *  Copyright (c) 2015                                                        *
 *     Universite de Versailles St-Quentin-en-Yvelines                        *
 *     CMLA, Ecole Normale Superieure de Cachan                               *
 *  Copyright (c) 2018                                                        *
 *     Universite de Versailles St-Quentin-en-Yvelines                        *
 *  Copyright (c) 2019                                                        *
 *     Verificarlo contributors                                               *
 *                                                                            *
 *  Verificarlo is free software: you can redistribute it and/or modify       *
 *  it under the terms of the GNU General Public License as published by      *
 *  the Free Software Foundation, either version 3 of the License, or         *
 *  (at your option) any later version.                                       *
 *                                                                            *
 *  Verificarlo is distributed in the hope that it will be useful,            *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 *  GNU General Public License for more details.                              *
 *                                                                            *
 *  You should have received a copy of the GNU General Public License         *
 *  along with Verificarlo.  If not, see <http://www.gnu.org/licenses/>.      *
 *                                                                            *
 ******************************************************************************/

#include "../../config.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <cxxabi.h>
#include <fstream>
#include <functional>
#include <regex>
#include <set>
#include <sstream>
#include <utility>

#if LLVM_VERSION_MAJOR < 5
#define CREATE_CALL3(func, op1, op2, op3)                                      \
  (Builder.CreateCall(func, {op1, op2, op3}, ""))
#define CREATE_CALL2(func, op1, op2) (Builder.CreateCall(func, {op1, op2}, ""))
#define CREATE_STRUCT_GEP(t, i, p) (Builder.CreateStructGEP(t, i, p, ""))
#define GET_OR_INSERT_FUNCTION(M, name, res, ...)                              \
  M.getOrInsertFunction(name, res, __VA_ARGS__, (Type *)NULL)
typedef llvm::Constant *_LLVMFunctionType;
#elif LLVM_VERSION_MAJOR < 9
#define CREATE_CALL3(func, op1, op2, op3)                                      \
  (Builder.CreateCall(func, {op1, op2, op3}, ""))
#define CREATE_CALL2(func, op1, op2) (Builder.CreateCall(func, {op1, op2}, ""))
#define CREATE_STRUCT_GEP(t, i, p) (Builder.CreateStructGEP(t, i, p, ""))
#define GET_OR_INSERT_FUNCTION(M, name, res, ...)                              \
  M.getOrInsertFunction(name, res, __VA_ARGS__)
typedef llvm::Constant *_LLVMFunctionType;
#else
#define CREATE_CALL3(func, op1, op2, op3)                                      \
  (Builder.CreateCall(func, {op1, op2, op3}, ""))
#define CREATE_CALL2(func, op1, op2) (Builder.CreateCall(func, {op1, op2}, ""))
#define CREATE_STRUCT_GEP(t, i, p) (Builder.CreateStructGEP(t, i, p, ""))
#define GET_OR_INSERT_FUNCTION(M, name, res, ...)                              \
  M.getOrInsertFunction(name, res, __VA_ARGS__)
typedef llvm::FunctionCallee _LLVMFunctionType;
#endif

using namespace llvm;
// VfclibInst pass command line arguments
static cl::opt<std::string>
    VfclibInstFunction("vfclibinst-function",
                       cl::desc("Only instrument given FunctionName"),
                       cl::value_desc("FunctionName"), cl::init(""));

static cl::opt<std::string> VfclibInstIncludeFile(
    "vfclibinst-include-file",
    cl::desc("Only instrument modules / functions in file IncludeNameFile "),
    cl::value_desc("IncludeNameFile"), cl::init(""));

static cl::opt<std::string> VfclibInstExcludeFile(
    "vfclibinst-exclude-file",
    cl::desc("Do not instrument modules / functions in file ExcludeNameFile "),
    cl::value_desc("ExcludeNameFile"), cl::init(""));

static cl::opt<bool> VfclibInstVerbose("vfclibinst-verbose",
                                       cl::desc("Activate verbose mode"),
                                       cl::value_desc("Verbose"),
                                       cl::init(false));

static cl::opt<bool>
    VfclibInstInstrumentFCMP("vfclibinst-inst-fcmp",
                             cl::desc("Instrument floating point comparisons"),
                             cl::value_desc("InstrumentFCMP"), cl::init(false));

namespace {
// Define an enum type to classify the floating points operations
// that are instrumented by verificarlo

enum Fops { FOP_ADD, FOP_SUB, FOP_MUL, FOP_DIV, FOP_CMP, FOP_IGNORE };

// Each instruction can be translated to a string representation

std::string Fops2str[] = {"add", "sub", "mul", "div", "cmp", "ignore"};

// Separtors for the module name
const char path_separator = '/';
const char relative_path_separator = '#';

struct VfclibInst : public ModulePass {
  static char ID;

  VfclibInst() : ModulePass(ID) {}

  // Taken from
  // https://www.fluentcpp.com/2017/04/21/how-to-split-a-string-in-c/
  std::vector<std::string> split(const std::string &s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
      tokens.push_back(token);
    }
    return tokens;
  }

  // taken from
  // https://stackoverflow.com/questions/281818/unmangling-the-result-of-stdtype-infoname
  std::string demangle(const std::string &name) {

    const char *name_c_str = name.c_str();
    int status = -4; // some arbitrary value to eliminate the compiler warning

    // enable c++11 by passing the flag -std=c++11 to g++
    std::unique_ptr<char, void (*)(void *)> res{
        abi::__cxa_demangle(name_c_str, NULL, NULL, &status), std::free};
    return (status == 0) ? res.get() : name;
  }

  // https://thispointer.com/find-and-replace-all-occurrences-of-a-sub-string-in-c/
  void findAndReplaceAll(std::string &data, std::string toSearch,
                         std::string replaceStr) {
    // Get the first occurrence
    size_t pos = data.find(toSearch);
    // Repeat till end is reached
    while (pos != std::string::npos) {
      // Replace this occurrence of Sub String
      data.replace(pos, toSearch.size(), replaceStr);
      // Get the next occurrence from the current position
      pos = data.find(toSearch, pos + replaceStr.size());
    }
  }

  void escape_regex(std::string &str) {
    findAndReplaceAll(str, ".", "\\.");
    // ECMAScript needs .* instead of * for matching any charactere
    // http://www.cplusplus.com/reference/regex/ECMAScript/
    findAndReplaceAll(str, "*", ".*");
  }

  std::string getSourceFileNameAbsPath(Module &M) {

    std::string filename = M.getSourceFileName();
    if (sys::path::is_absolute(filename))
      return filename;

    SmallString<4096> path;
    sys::fs::current_path(path);
    path.append("/" + filename);
    if (not sys::fs::make_absolute(path)) {
      return path.str().str();
    } else {
      return "";
    }
  }

  std::regex parseFunctionSetFile(Module &M, cl::opt<std::string> &fileName) {
    // Skip if empty fileName
    if (fileName.empty()) {
      return std::regex("");
    }

    // Open File
    std::ifstream loopstream(fileName.c_str());
    if (!loopstream.is_open()) {
      errs() << "Cannot open " << fileName << "\n";
      report_fatal_error("libVFCInstrument fatal error");
    }

    // Parse File, if module name matches, add function to FunctionSet
    int lineno = 0;
    std::string line;

    // return the absolute path of the source file
    std::string moduleName = getSourceFileNameAbsPath(M);
    moduleName = (moduleName.empty()) ? M.getModuleIdentifier() : moduleName;

    // Regex that contains all regex for each function
    std::string moduleRegex = "";

    while (std::getline(loopstream, line)) {
      lineno++;
      StringRef l = StringRef(line);

      // Ignore empty or commented lines
      if (l.startswith("#") || l.trim() == "") {
        continue;
      }
      std::pair<StringRef, StringRef> p = l.split(" ");

      if (p.second.equals("")) {
        errs() << "Syntax error in exclusion/inclusion file " << fileName << ":"
               << lineno << "\n";
        report_fatal_error("libVFCInstrument fatal error");
      } else {
        std::string mod = p.first.trim().str();
        std::string fun = p.second.trim().str();

        // If mod is not an absolute path,
        // we search any module containing mod
        if (sys::path::is_relative(mod)) {
          mod = "*" + sys::path::get_separator().str() + mod;
        }
        // If the user does not specify extension for the module
        // we match any extension
        if (not sys::path::has_extension(mod)) {
          mod += ".*";
        }

        escape_regex(mod);
        escape_regex(fun);

        if (std::regex_match(moduleName, std::regex(mod))) {
          moduleRegex += fun + "|";
        }
      }
    }

    loopstream.close();
    // Remove the extra | at the end
    if (not moduleRegex.empty()) {
      moduleRegex.pop_back();
    }
    return std::regex(moduleRegex);
  }

  bool runOnModule(Module &M) {
    bool modified = false;

    // Parse both included and excluded function set
    std::regex includeFunctionRgx =
        parseFunctionSetFile(M, VfclibInstIncludeFile);
    std::regex excludeFunctionRgx =
        parseFunctionSetFile(M, VfclibInstExcludeFile);

    // Parse instrument single function option (--function)
    if (not VfclibInstFunction.empty()) {
      includeFunctionRgx = std::regex(VfclibInstFunction);
      excludeFunctionRgx = std::regex(".*");
    }

    // Find the list of functions to instrument
    std::vector<Function *> functions;
    for (auto &F : M.functions()) {

      const std::string &name = F.getName().str();

      // Included-list
      if (std::regex_match(name, includeFunctionRgx)) {
        functions.push_back(&F);
        continue;
      }

      // Excluded-list
      if (std::regex_match(name, excludeFunctionRgx)) {
        continue;
      }

      // If excluded-list is empty and included-list is not, we are done
      if (VfclibInstExcludeFile.empty() and not VfclibInstIncludeFile.empty()) {
        continue;
      } else {
        // Everything else is neither include-listed or exclude-listed
        functions.push_back(&F);
      }
    }
    // Do the instrumentation on selected functions
    for (auto F : functions) {
      modified |= runOnFunction(M, *F);
    }
    // runOnModule must return true if the pass modifies the IR
    return modified;
  }

  bool runOnFunction(Module &M, Function &F) {
    if (VfclibInstVerbose) {
      errs() << "In Function: ";
      errs().write_escaped(demangle(F.getName().str())) << '\n';
    }

    bool modified = false;

    for (Function::iterator bi = F.begin(), be = F.end(); bi != be; ++bi) {
      modified |= runOnBasicBlock(M, *bi);
    }
    return modified;
  }

  Value *replaceWithMCACall(Module &M, Instruction *I, Fops opCode) {
    IRBuilder<> Builder(I);

    Type *opType = I->getOperand(0)->getType();
    Type *retType = I->getType();
    std::string opName = Fops2str[opCode];

    std::string baseTypeName = "";
    std::string vectorName = "";
    Type *baseType = opType;

    // Should we add a vector prefix?
    unsigned size = 1;
    if (opType->isVectorTy()) {
      VectorType *t = static_cast<VectorType *>(opType);
      baseType = t->getElementType();
      size = t->getNumElements();

      if (size == 2) {
        vectorName = "2x";
      } else if (size == 4) {
        vectorName = "4x";
      } else if (size == 8) {
        vectorName = "8x";
      } else if (size == 16) {
        vectorName = "16x";
      } else {
        errs() << "Unsuported vector size: " << size << "\n";
        return nullptr;
      }
    }

    // Check the type of the operation
    if (baseType->isDoubleTy()) {
      baseTypeName = "double";
    } else if (baseType->isFloatTy()) {
      baseTypeName = "float";
    } else {
      errs() << "Unsupported operand type: " << *opType << "\n";
      return nullptr;
    }

    // Build name of the helper function in vfcwrapper
    std::string mcaFunctionName = "_" + vectorName + baseTypeName + opName;

    // We call directly a hardcoded helper function
    // no need to go through the vtable at this stage.
    Value *newInst;
    if (opCode == FOP_CMP) {
      FCmpInst *FCI = static_cast<FCmpInst *>(I);
      Type *res = Builder.getInt32Ty();
      if (size > 1) {
        res = VectorType::get(res, size);
      }
      _LLVMFunctionType hookFunc = GET_OR_INSERT_FUNCTION(
          M, mcaFunctionName, res, Builder.getInt32Ty(), opType, opType);
      newInst = CREATE_CALL3(hookFunc, Builder.getInt32(FCI->getPredicate()),
                             FCI->getOperand(0), FCI->getOperand(1));
      newInst = Builder.CreateIntCast(newInst, retType, true);
    } else {
      _LLVMFunctionType hookFunc =
          GET_OR_INSERT_FUNCTION(M, mcaFunctionName, retType, opType, opType);
      newInst = CREATE_CALL2(hookFunc, I->getOperand(0), I->getOperand(1));
    }

    return newInst;
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
    case Instruction::FCmp:
      // Only instrument FCMP if the flag --inst-fcmp is passed
      if (VfclibInstInstrumentFCMP) {
        return FOP_CMP;
      } else {
        return FOP_IGNORE;
      }
    default:
      return FOP_IGNORE;
    }
  }

  bool runOnBasicBlock(Module &M, BasicBlock &B) {
    bool modified = false;
    std::set<std::pair<Instruction *, Fops>> WorkList;
    for (BasicBlock::iterator ii = B.begin(), ie = B.end(); ii != ie; ++ii) {
      Instruction &I = *ii;
      Fops opCode = mustReplace(I);
      if (opCode == FOP_IGNORE)
        continue;
      WorkList.insert(std::make_pair(&I, opCode));
    }

    for (auto p : WorkList) {
      Instruction *I = p.first;
      Fops opCode = p.second;
      if (VfclibInstVerbose)
        errs() << "Instrumenting" << *I << '\n';
      Value *value = replaceWithMCACall(M, I, opCode);
      if (value != nullptr) {
        BasicBlock::iterator ii(I);
        ReplaceInstWithValue(B.getInstList(), ii, value);
      }
      modified = true;
    }

    return modified;
  }
}; // namespace
} // namespace

char VfclibInst::ID = 0;
static RegisterPass<VfclibInst> X("vfclibinst", "verificarlo instrument pass",
                                  false, false);
