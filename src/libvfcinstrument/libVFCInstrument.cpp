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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#pragma GCC diagnostic pop

#include <cxxabi.h>
#include <fstream>
#include <functional>
#include <regex>
#include <set>
#include <sstream>
#include <utility>

#if LLVM_VERSION_MAJOR < 11
#define GET_VECTOR_TYPE(ty, size) VectorType::get(ty, size)
#else
#define GET_VECTOR_TYPE(ty, size) FixedVectorType::get(ty, size)
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

static cl::opt<std::string>
    VfclibInstVfcwrapper("vfclibinst-vfcwrapper-file",
                         cl::desc("Name of the vfcwrapper IR file "),
                         cl::value_desc("VfcwrapperIRFile"), cl::init(""));

static cl::opt<bool> VfclibInstVerbose("vfclibinst-verbose",
                                       cl::desc("Activate verbose mode"),
                                       cl::value_desc("Verbose"),
                                       cl::init(false));

static cl::opt<bool>
    VfclibInstInstrumentFCMP("vfclibinst-inst-fcmp",
                             cl::desc("Instrument floating point comparisons"),
                             cl::value_desc("InstrumentFCMP"), cl::init(false));

/* pointer that hold the vfcwrapper Module */
static Module *vfcwrapperM = nullptr;

namespace {
// Define an enum type to classify the floating points operations
// that are instrumented by verificarlo
enum Fops { FOP_ADD, FOP_SUB, FOP_MUL, FOP_DIV, FOP_CMP, FOP_IGNORE };

// Each instruction can be translated to a string representation
const std::string Fops2str[] = {"add", "sub", "mul", "div", "cmp", "ignore"};

// Separtors for the module name
const char path_separator = '/';
const char relative_path_separator = '#';

/* valid floating-point type to instrument */
std::map<Type::TypeID, std::string> validTypesMap = {
    std::pair<Type::TypeID, std::string>(Type::FloatTyID, "float"),
    std::pair<Type::TypeID, std::string>(Type::DoubleTyID, "double")};

/* valid vector sizes to instrument */
const std::set<unsigned> validVectorSizes = {2, 4, 8, 16};

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

  /* Load vfcwrapper.ll Module */
  void loadVfcwrapperIR(Module &M) {
    SMDiagnostic err;
    std::unique_ptr<Module> _M =
        parseIRFile(VfclibInstVfcwrapper, err, M.getContext());
    if (_M.get() == nullptr) {
      err.print(VfclibInstVfcwrapper.c_str(), errs());
      report_fatal_error("libVFCInstrument fatal error");
    }
    vfcwrapperM = _M.release();
  }

  bool runOnModule(Module &M) {
    bool modified = false;

    loadVfcwrapperIR(M);

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

  /* Constructs the mca function name */
  /* it is built as: */
  /*  _ <size>x<type><operation> for vector */
  /*   _<type><operation> for scalar */
  std::string getMCAFunctionName(Type *opType, Fops opCode) {
    std::string functionName;
    std::string size = "";

    Type *baseType = opType->getScalarType();
    if (VectorType *vecType = dyn_cast<VectorType>(opType)) {
      size = std::to_string(vecType->getNumElements()) + "x";
    }
    auto precision = validTypesMap[baseType->getTypeID()];
    auto operation = Fops2str[opCode];
    functionName = "_" + size + precision + operation;
    return functionName;
  }

  /* Check if Instruction I is a valid instruction to replace; scalar case */
  bool isValidScalarInstruction(Type *opType) {
    bool isValidType =
        validTypesMap.find(opType->getTypeID()) != validTypesMap.end();
    if (not isValidType) {
      errs() << "Unsupported operand type: " << *opType << "\n";
    }
    return isValidType;
  }

  /* Check if Instruction I is a valid instruction to replace; vector case */
  bool isValidVectorInstruction(Type *opType) {
    VectorType *vecType = static_cast<VectorType *>(opType);
    auto baseType = vecType->getScalarType();
    auto size = vecType->getNumElements();
    bool isValidSize = validVectorSizes.find(size) != validVectorSizes.end();
    if (not isValidSize) {
      errs() << "Unsuported vector size: " << size << "\n";
      return false;
    }
    return isValidScalarInstruction(baseType);
  }

  /* Check if Instruction I is a valid instruction to replace */
  bool isValidInstruction(Instruction *I) {
    Type *opType = I->getOperand(0)->getType();
    if (opType->isVectorTy()) {
      return isValidVectorInstruction(opType);
    } else {
      return isValidScalarInstruction(opType);
    }
  }

  bool runOnFunction(Module &M, Function &F) {
    if (VfclibInstVerbose) {
      errs() << "In Function: ";
      errs().write_escaped(F.getName().str()) << '\n';
    }

    bool modified = false;

    for (Function::iterator bi = F.begin(), be = F.end(); bi != be; ++bi) {
      modified |= runOnBasicBlock(M, *bi);
    }
    return modified;
  }

  Argument *getArgNo(Function *F, int argNo) {
    Argument *arg = nullptr;
#if LLVM_VERSION_MAJOR < 10
    int i = 0;
    for (auto &a : F->args()) {
      if (i++ == argNo) {
        arg = &a;
        break;
      }
    }
#else
    arg = F->getArg(argNo);
#endif
    return arg;
  }

  /* Add extra instructions for operand not matching the signature type */
  /* Returns the new operand */
  Value *updateOperand(IRBuilder<> &Builder, Function *F, Value *operand,
                       int argNo) {

    Type *opType = operand->getType();
    Argument *arg = getArgNo(F, argNo);
    Type *sigType = arg->getType();

    /* Check if operand and signature type match */
    if (opType != sigType) {
      if (CastInst::isBitCastable(opType, sigType)) {
        /* Small vectors can be optimzed like <2xfloat> can be casted in double
         */
        operand = Builder.CreateBitCast(operand, sigType);
      } else if (arg->hasByValAttr() and sigType->isPointerTy()) {
        /* If vectorial registers are not available */
        /* values are passed by pointer with attribute byval */
        AllocaInst *alloca = Builder.CreateAlloca(opType);
        Builder.CreateStore(operand, alloca);
        operand = alloca;
      }
    }
    return operand;
  }

  /* Update the return value if type mismatched */
  Value *updateReturn(IRBuilder<> &Builder, CallInst *newInst, Type *retType) {
    Type *retTypeNewInst = newInst->getType();
    if (retType != retTypeNewInst) {
      if (CastInst::isBitCastable(retTypeNewInst, retType)) {
        return Builder.CreateBitCast(newInst, retType);
      }
    }
    return newInst;
  }

  /* Replace arithmetic instructions with MCA */
  Value *replaceArithmeticWithMCACall(IRBuilder<> &Builder, Function *F,
                                      Instruction *I) {

    Value *op1 = I->getOperand(0);
    Value *op2 = I->getOperand(1);
    Type *retType = I->getType();

    op1 = updateOperand(Builder, F, op1, 0);
    op2 = updateOperand(Builder, F, op2, 1);

    CallInst *newInst = Builder.CreateCall(F, {op1, op2});
    newInst->setAttributes(F->getAttributes());

    newInst = dyn_cast<CallInst>(updateReturn(Builder, newInst, retType));

    return newInst;
  }

  /* Replace comparison instructions with MCA */
  Value *replaceComparisonWithMCACall(IRBuilder<> &Builder, Function *F,
                                      Instruction *I) {
    Type *opType = I->getOperand(0)->getType();
    Type *retType = I->getType();

    Value *op1 = I->getOperand(0);
    Value *op2 = I->getOperand(1);

    op1 = updateOperand(Builder, F, op1, 1);
    op2 = updateOperand(Builder, F, op2, 2);

    FCmpInst *FCI = static_cast<FCmpInst *>(I);
    Type *res = Builder.getInt32Ty();
    if (VectorType *vTy = dyn_cast<VectorType>(opType)) {
      auto size = vTy->getNumElements();
      res = GET_VECTOR_TYPE(res, size);
    }
    Value *newInst = Builder.CreateCall(
        F, {Builder.getInt32(FCI->getPredicate()), op1, op2});
    newInst = Builder.CreateIntCast(newInst, retType, true);
    return newInst;
  }

  /* Returns the MCA function */
  Function *getMCAFunction(Module &M, Type *opType, Fops opCode) {
    const std::string mcaFunctionName = getMCAFunctionName(opType, opCode);
    Function *vfcwrapperF = vfcwrapperM->getFunction(mcaFunctionName);
#if LLVM_VERSION_MAJOR < 9
    Constant *callee =
        M.getOrInsertFunction(mcaFunctionName, vfcwrapperF->getFunctionType(),
                              vfcwrapperF->getAttributes());
    Function *newVfcWrapperF = dyn_cast<Function>(callee);
#else
    FunctionCallee callee =
        M.getOrInsertFunction(mcaFunctionName, vfcwrapperF->getFunctionType(),
                              vfcwrapperF->getAttributes());
    Function *newVfcWrapperF = dyn_cast<Function>(callee.getCallee());
#endif
    return newVfcWrapperF;
  }

  // Returns true if the caller and the callee agree on how args will be passed
  // Available in TargetTransformInfoImpl since llvm-8
  bool areFunctionArgsABICompatible(Function *caller, Function *callee) {
    return (callee->getFnAttribute("target-features") !=
            caller->getFnAttribute("target-features")) and
           (callee->getFnAttribute("target-cpu") !=
            caller->getFnAttribute("target-cpu"));
  }

  Value *replaceWithMCACall(Module &M, Instruction *I, Fops opCode) {
    if (not isValidInstruction(I)) {
      return nullptr;
    }

    IRBuilder<> Builder(I);
    Type *opType = I->getOperand(0)->getType();

    Function *caller = I->getFunction();
    /* Get the mca function */
    Function *mcaFunction = getMCAFunction(M, opType, opCode);

    // If the caller and the callee (mcaFunction) have different ABI we set the
    // caller attributes to the callee ones.
    if (areFunctionArgsABICompatible(caller, mcaFunction)) {
      auto target_features = mcaFunction->getFnAttribute("target-features");
      auto target_cpu = mcaFunction->getFnAttribute("target-cpu");
      caller->addFnAttr(target_features);
      caller->addFnAttr(target_cpu);
    }

    // We call directly a hardcoded helper function
    // no need to go through the vtable at this stage.
    Value *newInst;
    if (opCode == FOP_CMP) {
      newInst = replaceComparisonWithMCACall(Builder, mcaFunction, I);
    } else {
      newInst = replaceArithmeticWithMCACall(Builder, mcaFunction, I);
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
