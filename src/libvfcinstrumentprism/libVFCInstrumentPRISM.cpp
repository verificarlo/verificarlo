/*****************************************************************************\
 *                                                                           *\
 *  This file is part of the Verificarlo project,                            *\
 *  under the Apache License v2.0 with LLVM Exceptions.                      *\
 *  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception.                 *\
 *  See https://llvm.org/LICENSE.txt for license information.                *\
 *                                                                           *\
 *                                                                           *\
 *  Copyright (c) 2015                                                       *\
 *     Universite de Versailles St-Quentin-en-Yvelines                       *\
 *     CMLA, Ecole Normale Superieure de Cachan                              *\
 *                                                                           *\
 *  Copyright (c) 2018                                                       *\
 *     Universite de Versailles St-Quentin-en-Yvelines                       *\
 *                                                                           *\
 *  Copyright (c) 2019-2026                                                  *\
 *     Verificarlo Contributors                                              *\
 *                                                                           *\
 ****************************************************************************/

#include "../../config.h"
#include <cstdint>
#include <memory>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Demangle/Demangle.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Module.h>
#ifdef PIC
#undef PIC
#endif
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Linker/Linker.h>
#include <llvm/TargetParser/SubtargetFeature.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/FileSystem.h>
#if LLVM_VERSION_MAJOR >= 18
#include <llvm/TargetParser/Host.h>
#else
#include <llvm/Support/Host.h>
#endif
#include <llvm/Support/Path.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

#include <llvm/IR/Mangler.h>
#pragma GCC diagnostic pop

#include <cmath>
#include <cxxabi.h>
#include <fstream>
#include <regex>

#include "TargetFeatures.hpp"
#include "libVFCInstrumentPRISMOptions.hpp"

#define FUNCTION_CALLEE FunctionCallee

#define VECTOR_TYPE FixedVectorType
#define GET_VECTOR_TYPE(ty, size) FixedVectorType::get(ty, size)
#define CREATE_FMA_CALL(Builder, type, args)                                   \
  Builder.CreateIntrinsic(Intrinsic::fma, type, args)
#define CREATE_VECTOR_ELEMENT_COUNT(size) ElementCount::getFixed(size)
#define GET_VECTOR_ELEMENT_COUNT(vecType)                                      \
  ((::llvm::FixedVectorType *)vecType)->getNumElements()

#if LLVM_VERSION_MAJOR >= 18
#define STARTS_WITH(str, prefix) str.starts_with(prefix)
#else
#define STARTS_WITH(str, prefix) str.startswith(prefix)
#endif

using namespace llvm;
// VfclibInst pass command line arguments

namespace {

[[noreturn]] void prism_fatal_error(const std::string &msg = "") {
  errs() << "=== fatal error ===\n";
  errs() << msg << "\n";
  report_fatal_error("libVFCInstrumentPRISM fatal error\n");
}

namespace fops {
// Define an enum type to classify the floating points operations
// that are instrumented by verificarlo
enum class type {
  ADD,
  SUB,
  MUL,
  DIV,
  FMA,
  CMP,
  IGNORE,
}; // namespace

// Each instruction can be translated to a string representation
auto getName(const type &ty) -> std::string {
  switch (ty) {
  case type::ADD:
    return "add";
  case type::SUB:
    return "sub";
  case type::MUL:
    return "mul";
  case type::DIV:
    return "div";
  case type::FMA:
    return "fma";
  case type::CMP:
    return "cmp";
  case type::IGNORE:
    return "ignore";
  }
  llvm_unreachable("unknown type");
}

auto isFMA(const Instruction *I) -> bool {
  if (isa<CallInst>(I)) {
    const auto *call = dyn_cast<CallInst>(I);
    if (call->getCalledFunction() != nullptr) {
      auto name = call->getCalledFunction()->getName();
      return (name.empty()) ? false : STARTS_WITH(name, "llvm.fma");
    }
  }
  return false;
}

auto getOpCode(const Instruction *I) -> type {
  switch (I->getOpcode()) {
  case Instruction::FAdd:
    return type::ADD;
  case Instruction::FSub:

    return type::SUB;
  case Instruction::FMul:

    return type::MUL;
  case Instruction::FDiv:

    return type::DIV;
  case Instruction::Call:
    return (isFMA(I)) ? type::FMA : type::IGNORE;
  default:
    return type::IGNORE;
  }
}

auto mustReplace(const Instruction &I) -> bool {
  return getOpCode(&I) != type::IGNORE;
}

auto getArity(const Instruction *I) -> uint32_t {
  switch (getOpCode(I)) {
  case type::ADD:
  case type::SUB:
  case type::MUL:
  case type::DIV:
    return 2;
  case type::FMA:
    return 3;
  case type::CMP:
    return 2;
  case type::IGNORE:
    return 0;
  }
  llvm_unreachable("unknown floating-point operation");
}

auto isValidFpType(const Type::TypeID &id) -> bool {
  return id == Type::FloatTyID || id == Type::DoubleTyID;
}

auto getFpTypeName(const Type::TypeID &id) -> std::string {
  if (id == Type::FloatTyID) {
    return "f32";
  }
  if (id == Type::DoubleTyID) {
    return "f64";
  }
  llvm_unreachable("unknown floating-point type");
}

auto isValidVectorSize(const unsigned &size) -> bool {
  return size == 2 || size == 4 || size == 8 || size == 16 || size == 32 ||
         size == 64;
}

} // namespace fops

using FPOps = fops::type;

std::set<std::string> functionsToExclude;

[[maybe_unused]] auto get_mangled_name(Function *F) -> std::string {
  // Create a Mangler
  llvm::Mangler Mang;

  // Get the mangled name
  std::string MangledName;
  llvm::raw_string_ostream RawOS(MangledName);
  Mang.getNameWithPrefix(RawOS, F, false);
  return MangledName;
}

auto get_demangled_name(const std::string &name) -> std::string {
  int status = 0;
  std::unique_ptr<char, decltype(&free)> demangled(
      abi::__cxa_demangle(name.c_str(), nullptr, nullptr, &status), &free);
  if (status == 0) {
    return std::string{demangled.get()};
  }
  return name;
}

enum class PrismPassingMode { ByValue, ByPointer };

auto PassingModeNamespace(PrismPassingMode mode) -> std::string {
  if (mode == PrismPassingMode::ByValue) {
    return "fixed";
  }
  if (mode == PrismPassingMode::ByPointer) {
    return "variable";
  }
  prism_fatal_error("Invalid passing mode");
}

class IRModule {
public:
  explicit IRModule(Module &M, const std::string &irFile) {
    libModule = loadVfcwrapperIR(M, irFile);
    // if ir is null, an error message has already been printed
    if (libModule == nullptr) {
      prism_fatal_error("fatal error while reading library IR: " + irFile);
    }
    getDemangledNames();
  }

  auto hasFunction(const std::string &name) -> bool {
    return demangledShortNamesToMangled.find(name) !=
           demangledShortNamesToMangled.end();
  }

  auto getFunction(const std::string &name) -> Function * {
    if (not hasFunction(name)) {
      return nullptr;
    }
    return libModule->getFunction(demangledShortNamesToMangled[name]);
  }

  auto copyFunction(Module *M, Function *F,
                    const std::string &functionName) -> FUNCTION_CALLEE {
    auto functionNameMangled = demangledShortNamesToMangled[functionName];

    return M->getOrInsertFunction(functionNameMangled, F->getFunctionType(),
                                  F->getAttributes());
  }

  void printDemangledNamesLibsSR() {
    errs() << "Demangled names\n";
    for (auto &p : demangledNamesToMangled) {
      if (p.first.find("N_") == p.first.npos) {
        errs() << p.first << " : " << p.second << "\n";
      }
      // errs() << p.first << " : " << p.second << "\n";
    }
  }

  void printDemangledNamesLibsSRShort() {
    errs() << "Short demangled names\n";
    for (auto &p : demangledShortNamesToMangled) {
      if (p.first.find("N_") == p.first.npos) {
        errs() << p.first << " : " << p.second << "\n";
      }
    }
  }

private:
  std::unique_ptr<Module> libModule = nullptr;
  std::map<std::string, std::string> demangledNamesToMangled;
  std::map<std::string, std::string> demangledShortNamesToMangled;

  void getDemangledNames() {
    for (auto &F : libModule->functions()) {
      if (F.isDeclaration()) {
        continue;
      }
      const std::string &mangled_name = F.getName().str();
      functionsToExclude.insert(mangled_name);

      std::string demangled_name = get_demangled_name(mangled_name);
      demangledNamesToMangled[demangled_name] = mangled_name;

      size_t parenPos = demangled_name.find('(');
      std::string demangled_name_short =
          (parenPos != std::string::npos) ? demangled_name.substr(0, parenPos)
                                          : demangled_name;

      demangledShortNamesToMangled[demangled_name_short] = mangled_name;
    }
    // printDemangledNamesLibsSRShort();
  }

  /* Load vfcwrapper.ll Module */
  static auto loadVfcwrapperIR(Module &M, const std::string &irFile)
      -> std::unique_ptr<Module> {
    SMDiagnostic err;
    auto newM = parseIRFile(irFile, err, M.getContext());
    if (newM == nullptr) {
      err.print(irFile.c_str(), errs());
      prism_fatal_error();
    }
    return newM;
  }
};

class PrismFunction {
private:
  Function *F;
  PrismPassingMode passing;

public:
  explicit PrismFunction(Function *F, PrismPassingMode passing)
      : F(F), passing(passing) {}

  [[nodiscard]] auto getName() const -> StringRef { return F->getName(); }
  [[nodiscard]] auto getFunction() const -> Function * { return F; }
  [[nodiscard]] auto getPassing() const -> PrismPassingMode { return passing; }

  [[nodiscard]] auto isByValue() const -> bool {
    return passing == PrismPassingMode::ByValue;
  }

  [[nodiscard]] auto isByPointer() const -> bool {
    return passing == PrismPassingMode::ByPointer;
  }

  [[nodiscard]] auto getReturnType() const -> Type * {
    return F->getReturnType();
  }

  [[nodiscard]] auto getOperandType(unsigned index) const -> Type * {
    return F->getFunctionType()->getParamType(index);
  }

  auto operator==(void *ptr) -> bool { return F == ptr; }
};

class PrismModule {
private:
  options::RoundingMode rounding_mode;
  options::DispatchMode dispatch_mode;
  IRModule staticLib;
  IRModule dynamicLib;

  auto getFunction(StringRef name) -> Function * {
    if (dispatch_mode.is_static()) {
      return staticLib.getFunction(name.str());
    }
    return dynamicLib.getFunction(name.str());
  }

  auto getCopyFunction(Module *M, Function *F,
                       const std::string &functionName) -> FUNCTION_CALLEE {
    if (dispatch_mode.is_static()) {
      return staticLib.copyFunction(M, F, functionName);
    }
    return dynamicLib.copyFunction(M, F, functionName);
  }

  static auto getFloatingPointTypeName(Type *Ty) -> std::string {
    if (Ty->isVectorTy()) {
      auto *vecType = dyn_cast<VectorType>(Ty);
      auto *baseType = vecType->getScalarType();
      auto size = GET_VECTOR_ELEMENT_COUNT(vecType);
      auto base_type_name = fops::getFpTypeName(baseType->getTypeID());
      return base_type_name + "x" + std::to_string(size);
    }
    return fops::getFpTypeName(Ty->getTypeID());
  }

  auto getFunctionNameScalar(Instruction *I, FPOps opCode) -> std::string {
    const auto mode = rounding_mode.get_namespace();
    const auto dispatch = dispatch_mode.get_namespace();
    const auto opname = fops::getName(opCode);
    const auto fpname = getFloatingPointTypeName(I->getType());
    const auto fname = opname + fpname;

    return "prism::" + mode + "::scalar::" + dispatch + "::" + fname;
  }

  auto
  getFunctionNameVector(Instruction *I, FPOps opCode,
                        const PrismPassingMode &passing_style) -> std::string {
    const auto mode = rounding_mode.get_namespace();
    const auto dispatch = dispatch_mode.get_namespace();
    const auto passing = PassingModeNamespace(passing_style);
    const auto opname = fops::getName(opCode);
    const auto fpname = getFloatingPointTypeName(I->getType());
    const auto fname = opname + fpname;

    return "prism::" + mode + "::vector::" + dispatch + "::" + passing +
           "::" + fname;
  }

  auto getFunctionName(Instruction *I, FPOps opCode,
                       const PrismPassingMode &passing) -> std::string {

    if (opCode == FPOps::IGNORE) {
      prism_fatal_error("Unsupported opcode: " + fops::getName(opCode));
    }

    auto *baseType = I->getType();

    if (baseType->isVectorTy()) {
      return getFunctionNameVector(I, opCode, passing);
    }

    return getFunctionNameScalar(I, opCode);
  }

public:
  explicit PrismModule(Module &M, options::RoundingMode rounding_mode,
                       options::DispatchMode dispatch_mode,
                       const cl::opt<std::string> &VfclibInstStaticIRFile,
                       const cl::opt<std::string> &VfclibInstDynamicIRFile)
      : rounding_mode(std::move(rounding_mode)),
        dispatch_mode(std::move(dispatch_mode)),
        staticLib(IRModule(M, VfclibInstStaticIRFile)),
        dynamicLib(M, VfclibInstDynamicIRFile) {}

  // return the corresponding prism function for the given instruction
  auto getPrismFunction(Instruction *I, const FPOps &opcode) -> PrismFunction {

    PrismPassingMode passing_style = PrismPassingMode::ByValue;

    // try to find implementation for fixed vector length if exists
    auto functionName = getFunctionName(I, opcode, PrismPassingMode::ByValue);
    Function *function = getFunction(functionName);
    passing_style = PrismPassingMode::ByValue;

    // otherwise try to find implementation for variable vector length
    if (function == nullptr) {
      functionName = getFunctionName(I, opcode, PrismPassingMode::ByPointer);
      function = getFunction(functionName);
      passing_style = PrismPassingMode::ByPointer;
    }

    if (function == nullptr) {
      if (VfclibInstStrictABI) {
        prism_fatal_error("Function not found: " + functionName);
      } else {
        errs() << "Warning: Function not found: " << functionName << "\n";
        return PrismFunction(nullptr, passing_style);
      }
    }

    auto F = getCopyFunction(I->getModule(), function, functionName);
    auto *prism_wrapper_function = dyn_cast<Function>(F.getCallee());
    return PrismFunction(prism_wrapper_function, passing_style);
  }
};

struct VfclibInst : public ModulePass {
  static char ID;
  std::unique_ptr<PrismModule> prismModule = nullptr;
  constexpr static int returnIndex = -1;
  using prismAllocaKey = std::tuple<Function *, Type *, int>;
  std::map<prismAllocaKey, Instruction *> prismAllocaMap;
  const options::RoundingMode rounding_mode =
      options::RoundingMode(VfclibInstMode);
  const options::DispatchMode dispatch_mode =
      options::DispatchMode(VfclibInstDispatch);
  const bool debug_operands =
      VfclibInstDebugOptions.isSet(debugOptions::GetOperands);

  VfclibInst() : ModulePass(ID) {

    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();
  }

  // https://thispointer.com/find-and-replace-all-occurrences-of-a-sub-string-in-c/
  static void findAndReplaceAll(std::string &data, const std::string &toSearch,
                                const std::string &replaceStr) {
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

  static void escape_regex(std::string &str) {
    findAndReplaceAll(str, ".", "\\.");
    // ECMAScript needs .* instead of * for matching any charactere
    // http://www.cplusplus.com/reference/regex/ECMAScript/
    findAndReplaceAll(str, "*", ".*");
  }

  static auto getSourceFileNameAbsPath(Module &M) -> std::string {
    std::string filename = M.getSourceFileName();
    if (sys::path::is_absolute(filename)) {
      return filename;
    }

    SmallString<4096> path;
    if (sys::fs::current_path(path)) {
      prism_fatal_error("Error getting current path");
    }
    path.append("/" + filename);
    if (not sys::fs::make_absolute(path)) {
      return path.str().str();
    }
    return "";
  }

  // TODO(yohan): raise a clean error if the file is not well formatted
  static auto
  parseFunctionSetFile(Module &M,
                       const cl::opt<std::string> &fileName) -> std::regex {
    // Skip if empty fileName
    if (fileName.empty()) {
      return std::regex("");
    }

    // Open File
    std::ifstream loopstream(fileName.c_str());
    if (!loopstream.is_open()) {
      prism_fatal_error("Cannot open " + fileName);
    }

    // Parse File, if module name matches, add function to FunctionSet
    int lineno = 0;
    std::string line;

    // return the absolute path of the source file
    std::string moduleName = getSourceFileNameAbsPath(M);
    moduleName = (moduleName.empty()) ? M.getModuleIdentifier() : moduleName;

    // Regex that contains all regex for each function
    std::string moduleRegex;

    while (std::getline(loopstream, line)) {
      lineno++;
      auto l = StringRef(line);

      // Ignore empty or commented lines
      if (STARTS_WITH(l, "#") || l.trim() == "") {
        continue;
      }
      std::pair<StringRef, StringRef> p = l.split(" ");

      if (p.second.empty()) {
        prism_fatal_error("Syntax error in exclusion/inclusion file " +
                          fileName + ":" + std::to_string(lineno));
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

  auto runOnModule(Module &M) -> bool override {
    bool modified = false;

    prismModule = std::make_unique<PrismModule>(
        PrismModule(M, rounding_mode, dispatch_mode, VfclibInstStaticIRFile,
                    VfclibInstDynamicIRFile));

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

      // Function in the sr library to exclude
      if (functionsToExclude.find(name) != functionsToExclude.end()) {
        continue;
      }

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
      }

      // Everything else is neither include-listed or exclude-listed
      functions.push_back(&F);
    }

    // Do the instrumentation on selected functions
    for (auto *F : functions) {
      modified |= runOnFunction(M, *F);
    }

    return modified;
  }

  // runOnModule must return true if the pass modifies the IR
  static auto isValidScalarInstruction(Type *opType) -> bool {
    return fops::isValidFpType(opType->getTypeID());
  }

  /* Check if Instruction I is a valid instruction to replace; vector case */
  auto isValidVectorInstruction(Type *opType) -> bool {
    if (opType == nullptr) {
      errs() << "Unsupported operand type\n";
    }
    auto *vecType = dyn_cast<VectorType>(opType);
    auto *baseType = vecType->getScalarType();

    if (isa<ScalableVectorType>(vecType))
      prism_fatal_error("Scalable vector type are not supported");
    auto size = ((::llvm::FixedVectorType *)vecType)->getNumElements();
    bool isValidSize = fops::isValidVectorSize(size);
    if (not isValidSize) {
      errs() << "Unsuported vector size: " << size << "\n";
      return false;
    }
    return isValidScalarInstruction(baseType);
  }

  /* Check if Instruction I is a valid instruction to replace */
  auto isValidInstruction(Instruction *I) -> bool {
    Type *opType = I->getOperand(0)->getType();
    if (opType->isVectorTy()) {
      return isValidVectorInstruction(opType);
    }
    return isValidScalarInstruction(opType);
  }

  auto runOnFunction(Module &M, Function &F) -> bool {
    if (VfclibInstVerbose) {
      errs() << "In Function: ";
      errs().write_escaped(F.getName().str()) << '\n';
    }

    bool modified = false;

    for (auto &bi : F) {
      modified |= runOnBasicBlock(M, bi);
    }
    return modified;
  }

  static auto createFunction(FunctionType *functionType, Module &M,
                             const std::string &name) -> Function * {
    return Function::Create(functionType, Function::ExternalLinkage, name, M);
  }

  static auto getFPTypeName(Type *Ty) -> std::string {
    if (Ty->isVectorTy()) {
      auto *vecType = dyn_cast<VectorType>(Ty);
      auto *baseType = vecType->getScalarType();
      auto size = GET_VECTOR_ELEMENT_COUNT(vecType);
      const auto base_type_name = fops::getFpTypeName(baseType->getTypeID());
      return base_type_name + "x" + std::to_string(size);
    }
    return fops::getFpTypeName(Ty->getTypeID());
  }

  auto getPrismFunction(Instruction *I) -> PrismFunction {
    return prismModule->getPrismFunction(I, fops::getOpCode(I));
  }

  static auto isf32x2Value(Value *V) -> bool {
    if (not V->getType()->isVectorTy()) {
      return false;
    }
    auto *vecType = dyn_cast<VectorType>(V->getType());
    auto *baseType = vecType->getScalarType();
    auto size = GET_VECTOR_ELEMENT_COUNT(vecType);
    return baseType->isFloatTy() and size == 2;
  }

  static auto f32x2ToDoubleCast(IRBuilder<> &Builder, Value *V) -> Value * {
    return Builder.CreateBitCast(V, Builder.getDoubleTy());
  }

  auto getAllocaForVec(Value *V, Function *parent,
                       const uint32_t argIndex) -> Value * {
    auto *type = V->getType();

    const auto key = prismAllocaKey(parent, type, argIndex);
    if (prismAllocaMap.find(key) == prismAllocaMap.end()) {
      // create a new alloca at the beginning of the function
      IRBuilder<> Builder(&parent->getEntryBlock(),
                          parent->getEntryBlock().begin());

      auto *vecType = dyn_cast<VectorType>(type);
      const auto vecSize = GET_VECTOR_ELEMENT_COUNT(vecType);
      const auto ftype = "f" + std::to_string(type->getScalarSizeInBits());
      const auto size = "x" + std::to_string(vecSize);
      const auto index =
          (argIndex != returnIndex) ? std::to_string(argIndex) : "ret";
      const auto name = "prism_alloca_" + ftype + "_" + size + "_" + index;
      auto *alloca = Builder.CreateAlloca(type, nullptr, name);
      prismAllocaMap[key] = alloca;
    }

    return prismAllocaMap[key];
  }

  auto getOperandVector(IRBuilder<> &Builder, Instruction *I, uint32_t argIndex,
                        const PrismFunction &F) -> Value * {

    auto *parent_function = I->getParent()->getParent();
    auto *op = I->getOperand(argIndex);
    auto *arg_type = F.getFunction()->getFunctionType()->getParamType(argIndex);

    if (debug_operands) {
      errs() << "Operand " << argIndex << ": " << *op << "\n";
      errs() << "Arg type " << argIndex << ": " << *arg_type << "\n";
    }

    Value *operand = nullptr;

    if (F.isByPointer() and arg_type->isPointerTy()) {
      // dynamic dispatch with pointer arguments
      // if the vector is passed as a value
      // need to create an alloca and store the value

      auto *alloca = getAllocaForVec(op, parent_function, argIndex);
      Builder.CreateStore(op, alloca);
      auto *bitcast =
          Builder.CreateBitCast(alloca, arg_type, "prism_bitcast_arg");
      operand = bitcast;

      if (debug_operands) {
        errs() << "Operand " << argIndex << " after alloca: " << *alloca
               << "\n";
      }

    } else if (isf32x2Value(op)) {
      op = f32x2ToDoubleCast(Builder, op);
      operand = op;

      if (debug_operands) {
        errs() << "Operand " << argIndex << " after cast: " << *op << "\n";
      }

    } else { // static dispatch
      operand = op;
    }

    return operand;
  }

  auto getOperandScalar(Instruction *I, uint32_t argIndex,
                        const PrismFunction &F) const -> Value * {

    auto *op = I->getOperand(argIndex);
    auto *arg_type = F.getFunction()->getFunctionType()->getParamType(argIndex);

    if (debug_operands) {
      errs() << "Operand " << argIndex << ": " << *op << "\n";
      errs() << "Arg type " << argIndex << ": " << *arg_type << "\n";
    }

    Value *operand = op;

    return operand;
  }

  auto getOperands(IRBuilder<> &Builder, Instruction *I,
                   const PrismFunction &F) -> std::vector<Value *> {
    if (debug_operands) {
      errs() << "Get operands for function: "
             << get_demangled_name(F.getName().str()) << "\n";
      errs() << "\n" << *(F.getFunction()) << "\n";
    }

    bool isVectorInstruction = I->getType()->isVectorTy();

    std::vector<Value *> operands;
    for (unsigned i = 0; i < fops::getArity(I); i++) {
      Value *operand = nullptr;
      if (isVectorInstruction) {
        operand = getOperandVector(Builder, I, i, F);
      } else {
        operand = getOperandScalar(I, i, F);
      }
      operands.push_back(operand);
    }

    auto *parent_function = I->getParent()->getParent();

    if (F.isByPointer() and isVectorInstruction) {
      // dynamic dispatch with pointer return value
      // auto alloca = Builder.CreateAlloca(I->getType(), nullptr,
      // "prism_alloca_result");
      auto *alloca = getAllocaForVec(I, parent_function, returnIndex);
      // get type of last argument of F
      auto *arg_type = F.getOperandType(fops::getArity(I));
      auto *bitcast =
          Builder.CreateBitCast(alloca, arg_type, "prism_bitcast_result");
      operands.push_back(bitcast);

      if (debug_operands) {
        errs() << "Result alloca: " << *alloca << "\n";
      }
    }

    return operands;
  }

  static auto getReturnValueVector(IRBuilder<> &Builder, Instruction *Old,
                                   Value *Return, Instruction *New,
                                   const PrismFunction &F) -> Value * {
    Value *result = nullptr;
    if (F.isByPointer()) {
      // pointer return value
      auto *return_type = Old->getType();
      auto *return_type_ptr = return_type->getPointerTo();
      auto *ptr_to_vec = Builder.CreateBitCast(Return, return_type_ptr);
      auto *load = Builder.CreateLoad(return_type, ptr_to_vec);
      result = load;
    } else {
      auto *return_type = F.getReturnType();
      if (isf32x2Value(Old) and return_type->isDoubleTy()) {
        auto *bitcast = Builder.CreateBitCast(New, Old->getType());
        result = bitcast;
      } else {
        result = New;
      }
    }
    return result;
  }

  /* Replace arithmetic instructions with PR */
  auto replaceArithmeticWithPRCall(IRBuilder<> &Builder,
                                   Instruction *I) -> Value * {
    auto F = getPrismFunction(I);
    if (F.getFunction() == nullptr) {
      // Skip instrumentation if the function is missing
      return nullptr;
    }

    if (not areABICompatible(I, F.getFunction())) {
      return nullptr;
    }

    auto operands = getOperands(Builder, I, F);

    auto *call = Builder.CreateCall(F.getFunction(), operands);
    call->setAttributes(F.getFunction()->getAttributes());

    Value *result = nullptr;

    const bool isVectorInstruction = I->getType()->isVectorTy();

    if (isVectorInstruction) {
      result = getReturnValueVector(Builder, I, operands.back(), call, F);
    } else {
      result = call;
    }

    return result;
  }

  auto replaceWithPRCall(Instruction *I) -> Value * {
    if (not isValidInstruction(I)) {
      return nullptr;
    }

    IRBuilder<> Builder(I);

    // Check if this instruction is the last one in the block
    if (I->isTerminator()) {
      // Insert at the end of the block
      Builder.SetInsertPoint(I->getParent());
    } else {
      // Set insertion point after the given instruction
      Builder.SetInsertPoint(I->getNextNode());
    }

    Value *newInst = replaceArithmeticWithPRCall(Builder, I);

    return newInst;
  }

  auto runOnBasicBlock(Module &M, BasicBlock &B) -> bool {
    bool modified = false;
    std::set<Instruction *> WorkList;
    for (auto &I : B) {
      if (fops::mustReplace(I)) {
        WorkList.insert(&I);
      }
    }

    for (auto *I : WorkList) {
      Value *value = replaceWithPRCall(I);
      if (value != nullptr) {
        if (VfclibInstVerbose) {
          errs() << "Instrumenting" << *I << '\n';
        }
        BasicBlock::iterator ii(I);
        ReplaceInstWithValue(ii, value);
      }
    }

    return modified;
  }
}; // namespace
} // namespace

char VfclibInst::ID = 0;
static RegisterPass<VfclibInst> X("vfclibinst", "verificarlo instrument pass",
                                  false, false);

namespace {
struct VfclibInstPass : public PassInfoMixin<VfclibInstPass> {
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &) {
    VfclibInst legacy;
    legacy.runOnModule(M);
    return PreservedAnalyses::none();
  }
};
} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "vfclibinst", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "vfclibinst") {
                    MPM.addPass(VfclibInstPass());
                    return true;
                  }
                  return false;
                });
          }};
}
