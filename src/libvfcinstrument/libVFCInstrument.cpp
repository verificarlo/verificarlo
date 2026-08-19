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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#ifdef PIC
#undef PIC
#endif
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TargetParser/Triple.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <llvm/Support/SourceMgr.h>
#pragma GCC diagnostic pop

#include <cxxabi.h>
#include <fstream>
#include <functional>
#include <regex>
#include <set>
#include <sstream>
#include <utility>

#define GET_VECTOR_TYPE(ty, size) FixedVectorType::get(ty, size)

#if LLVM_VERSION_MAJOR >= 18
#define STARTS_WITH(str, prefix) str.starts_with(prefix)
#else
#define STARTS_WITH(str, prefix) str.startswith(prefix)
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

static cl::opt<bool>
    VfclibInstInstrumentFMA("vfclibinst-inst-fma",
                            cl::desc("Instrument floating point fma"),
                            cl::value_desc("InstrumentFMA"), cl::init(false));

static cl::opt<bool> VfclibInstInstrumentCast(
    "vfclibinst-inst-cast",
    cl::desc("Instrument floating point cast instructions"),
    cl::value_desc("InstrumentCast"), cl::init(false));

namespace {
// Define an enum type to classify the floating points operations
// that are instrumented by verificarlo
enum FPOps {
  FOP_ADD,
  FOP_SUB,
  FOP_MUL,
  FOP_DIV,
  FOP_CMP,
  FOP_FMA,
  FOP_CAST,
  FOP_IGNORE
};

// Each instruction can be translated to a string representation
const std::string Fops2str[] = {"add", "sub", "mul",  "div",
                                "cmp", "fma", "cast", "ignore"};

/* valid floating-point type to instrument */
std::map<Type::TypeID, std::string> validTypesMap = {
    {Type::FloatTyID, "float"}, {Type::DoubleTyID, "double"}};

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
      if (STARTS_WITH(l, "#") || l.trim().empty()) {
        continue;
      }
      std::pair<StringRef, StringRef> p = l.split(" ");

      if (p.second.empty()) {
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

  /* Constructs the mca function name */
  /* it is built as: */
  /*  _ <size>x<type><operation> for vector */
  /*   _<type><operation> for scalar */
  std::string getMCAFunctionName(Instruction *I, FPOps opCode) {
    std::string functionName;
    std::string size = "";

    Type *opType = I->getOperand(0)->getType();
    Type *baseType = opType->getScalarType();
    if (VectorType *vecType = dyn_cast<VectorType>(opType)) {
      if (isa<ScalableVectorType>(vecType))
        report_fatal_error("Scalable vector type are not supported");
      size = std::to_string(
                 ((::llvm::FixedVectorType *)vecType)->getNumElements()) +
             "x";
    }

    std::string precision;
    if (opCode == FOP_CAST) {
      auto srcTyName = validTypesMap[I->getOperand(0)->getType()->getTypeID()];
      auto dstTyName = validTypesMap[I->getType()->getTypeID()];
      precision = srcTyName + "to" + dstTyName;
    } else {
      precision = validTypesMap[baseType->getTypeID()];
    }

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
    if (isa<ScalableVectorType>(vecType))
      report_fatal_error("Scalable vector type are not supported");
    auto size = ((::llvm::FixedVectorType *)vecType)->getNumElements();
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
    arg = F->getArg(argNo);
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
        /* Small vectors can be optimzed, .i.e <2xfloat> can be casted in double
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

    return updateReturn(Builder, newInst, retType);
  }

  /* Replace fma arithmetic instructions with MCA */
  Value *replaceArithmeticFMAWithMCACall(IRBuilder<> &Builder, Function *F,
                                         Instruction *I) {

    Value *op1 = I->getOperand(0);
    Value *op2 = I->getOperand(1);
    Value *op3 = I->getOperand(2);

    Type *retType = I->getType();

    op1 = updateOperand(Builder, F, op1, 0);
    op2 = updateOperand(Builder, F, op2, 1);
    op3 = updateOperand(Builder, F, op3, 2);

    CallInst *newInst = Builder.CreateCall(F, {op1, op2, op3});
    newInst->setAttributes(F->getAttributes());

    return updateReturn(Builder, newInst, retType);
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
      if (isa<ScalableVectorType>(vTy))
        report_fatal_error("Scalable vector type are not supported");
      auto size = ((::llvm::FixedVectorType *)vTy)->getNumElements();
      res = GET_VECTOR_TYPE(res, size);
    }
    Value *newInst = Builder.CreateCall(
        F, {Builder.getInt32(FCI->getPredicate()), op1, op2});
    if (newInst->getType() != res) {
      if (CastInst::isBitCastable(newInst->getType(), res)) {
        newInst = Builder.CreateBitCast(newInst, res);
      }
    }
    if (newInst->getType() == retType) {
      return newInst;
    }
    return Builder.CreateIntCast(newInst, retType, true);
  }

  /* Replace cast instruction with MCA */
  Value *replaceCastWithMCACall(IRBuilder<> &Builder, Function *F,
                                Instruction *I) {
    Type *dstType = I->getType();
    Value *op = I->getOperand(0);
    op = updateOperand(Builder, F, op, 0);
    CallInst *newInst = Builder.CreateCall(F, {op});
    newInst->setAttributes(F->getAttributes());
    return updateReturn(Builder, newInst, dstType);
  }

  /* Returns the MCA function declaration.
   *
   * For vector operations the pass selects an ISA-specific symbol
   * (e.g. "_4xfloatadd_avx2") from the caller target-features and synthesizes
   * the ABI-correct LLVM declaration directly.
   *
   * Scalar operations always use the generic scalar dispatch wrapper.
   */
  Function *getMCAFunction(Module &M, Instruction *I, FPOps opCode) {
    std::string name = getMCAFunctionName(I, opCode);
    std::string isaSuffix;

    // For vector operations attempt an ISA-specific lookup first
    bool isVector = I->getOperand(0)->getType()->isVectorTy();
    if (isVector and opCode != FOP_CAST) {
      isaSuffix = getISASuffix(I->getFunction());
      if (not isaSuffix.empty()) {
        name += isaSuffix;
      }
    }
    return createMCAFunctionDeclaration(M, name, I, opCode, isaSuffix);
  }

  // Returns true if the caller and the callee do NOT agree on how args will be
  // passed. Available in TargetTransformInfoImpl since llvm-8
  bool areFunctionArgsABICompatible(Function *caller, Function *callee) {
    if (callee->getReturnType() != caller->getReturnType()) {
      return true;
    }
    if (callee->getFunctionType()->getNumParams() == 0 ||
        caller->getFunctionType()->getNumParams() == 0) {
      return false;
    }
    return callee->getFunctionType()->getParamType(0) !=
           caller->getFunctionType()->getParamType(0);
  }

  /* Returns the ISA suffix for the caller's target-features.
   *
   * Checking order follows hardware capability hierarchy so the most capable
   * level wins.
   *
   * x86:  +avx512f → "_avx512"  |  +avx2 → "_avx2"  |  +avx → "_avx"
   *       otherwise x86-64 defaults to "_sse2"
   * ARM:  +sve2 → "_sve2"  |  +sve → "_sve"  |  +neon → "_neon"
   */
  std::string getISASuffix(Function *caller) {
    Triple triple(caller->getParent()->getTargetTriple());

    Attribute targetFeatures = caller->getFnAttribute("target-features");
    StringRef features = targetFeatures.isStringAttribute()
                             ? targetFeatures.getValueAsString()
                             : StringRef();

    // x86 — ordered from most to least capable
    if (triple.isX86()) {
      if (features.find("+avx512f") != StringRef::npos)
        return "_avx512";
      if (features.find("+avx2") != StringRef::npos)
        return "_avx2";
      if (features.find("+avx") != StringRef::npos)
        return "_avx";
      return "_sse2";
    }

    // ARM AArch64 — ordered from most to least capable
    if (!triple.isAArch64())
      return "";
    if (features.find("+sve2") != StringRef::npos)
      return "_sve2";
    if (features.find("+sve") != StringRef::npos)
      return "_sve";
    if (features.find("+neon") != StringRef::npos)
      return "_neon";

    return "";
  }

  unsigned getMaxDirectVectorWidth(Module &M, StringRef isaSuffix) {
    Triple triple(M.getTargetTriple());
    if (triple.isX86()) {
      if (isaSuffix == "_avx512")
        return 512;
      if (isaSuffix == "_avx2" || isaSuffix == "_avx")
        return 256;
      return 128;
    }

    if (triple.isAArch64()) {
      if (isaSuffix == "_sve2" || isaSuffix == "_sve")
        return 0;
      return 128;
    }

    return 0;
  }

  bool shouldPassIndirectly(Type *Ty, Module &M, StringRef isaSuffix) {
    auto *vecTy = dyn_cast<FixedVectorType>(Ty);
    if (vecTy == nullptr)
      return false;

    unsigned maxDirectWidth = getMaxDirectVectorWidth(M, isaSuffix);
    if (maxDirectWidth == 0)
      return false;

    return vecTy->getPrimitiveSizeInBits().getFixedValue() > maxDirectWidth;
  }

  Type *getABIType(Type *Ty, Module &M) {
    auto *vecTy = dyn_cast<FixedVectorType>(Ty);
    if (vecTy == nullptr)
      return Ty;

    Triple triple(M.getTargetTriple());
    if (!triple.isX86())
      return Ty;

    auto bits = vecTy->getPrimitiveSizeInBits().getFixedValue();
    if (bits == 64) {
      return Type::getDoubleTy(M.getContext());
    }
    return Ty;
  }

  void addParameter(Type *sourceType, std::vector<Type *> &paramTypes,
                    std::vector<std::pair<unsigned, Type *>> &byValParams,
                    Module &M, StringRef isaSuffix) {
    if (shouldPassIndirectly(sourceType, M, isaSuffix)) {
      unsigned index = paramTypes.size();
      paramTypes.push_back(PointerType::getUnqual(M.getContext()));
      byValParams.push_back({index, sourceType});
      return;
    }
    paramTypes.push_back(getABIType(sourceType, M));
  }

  void addTargetAttributes(Function *F, StringRef isaSuffix) {
    if (isaSuffix.empty())
      return;

    if (isaSuffix == "_avx512") {
      F->addFnAttr("target-features", "+avx512f");
    } else if (isaSuffix == "_avx2") {
      F->addFnAttr("target-features", "+avx,+avx2");
    } else if (isaSuffix == "_avx") {
      F->addFnAttr("target-features", "+avx");
    } else if (isaSuffix == "_sse2") {
      F->addFnAttr("target-features", "+sse2");
    } else if (isaSuffix == "_sve2") {
      F->addFnAttr("target-features", "+sve2");
    } else if (isaSuffix == "_sve") {
      F->addFnAttr("target-features", "+sve");
    } else if (isaSuffix == "_neon") {
      F->addFnAttr("target-features", "+neon");
    }
  }

  Type *getComparisonResultType(Instruction *I, Module &M) {
    Type *opType = I->getOperand(0)->getType();
    Type *resultType = Type::getInt32Ty(M.getContext());

    if (VectorType *vTy = dyn_cast<VectorType>(opType)) {
      if (isa<ScalableVectorType>(vTy))
        report_fatal_error("Scalable vector type are not supported");
      auto size = cast<FixedVectorType>(vTy)->getNumElements();
      resultType = GET_VECTOR_TYPE(resultType, size);
    }

    return getABIType(resultType, M);
  }

  bool shouldScalarizeVectorReturn(Module &M, Instruction *I, FPOps opCode) {
    if (opCode == FOP_CAST || !I->getOperand(0)->getType()->isVectorTy())
      return false;

    std::string isaSuffix = getISASuffix(I->getFunction());
    unsigned maxDirectWidth = getMaxDirectVectorWidth(M, isaSuffix);
    if (maxDirectWidth == 0)
      return false;

    Type *resultType = opCode == FOP_CMP ? getComparisonResultType(I, M)
                                         : getABIType(I->getType(), M);
    auto *vecTy = dyn_cast<FixedVectorType>(resultType);
    return vecTy != nullptr &&
           vecTy->getPrimitiveSizeInBits().getFixedValue() > maxDirectWidth;
  }

  Value *replaceWithScalarizedMCACalls(Module &M, IRBuilder<> &Builder,
                                       Instruction *I, FPOps opCode) {
    auto *vectorType = cast<FixedVectorType>(I->getOperand(0)->getType());
    Type *scalarType = vectorType->getElementType();
    unsigned lanes = vectorType->getNumElements();

    std::vector<Type *> parameterTypes;
    if (opCode == FOP_CMP)
      parameterTypes.push_back(Type::getInt32Ty(M.getContext()));
    parameterTypes.push_back(scalarType);
    parameterTypes.push_back(scalarType);
    if (opCode == FOP_FMA)
      parameterTypes.push_back(scalarType);

    Type *scalarResultType =
        opCode == FOP_CMP ? Type::getInt32Ty(M.getContext()) : scalarType;
    std::string scalarName =
        "_" + validTypesMap[scalarType->getTypeID()] + Fops2str[opCode];
    FunctionType *functionType =
        FunctionType::get(scalarResultType, parameterTypes, false);
    FunctionCallee scalarFunction =
        M.getOrInsertFunction(scalarName, functionType);

    Type *resultType = opCode == FOP_CMP
                           ? FixedVectorType::get(scalarResultType, lanes)
                           : I->getType();
    Value *result = UndefValue::get(resultType);
    for (unsigned lane = 0; lane < lanes; lane++) {
      Value *laneIndex = Builder.getInt32(lane);
      std::vector<Value *> arguments;
      if (opCode == FOP_CMP) {
        auto *comparison = cast<FCmpInst>(I);
        arguments.push_back(Builder.getInt32(comparison->getPredicate()));
      }
      arguments.push_back(
          Builder.CreateExtractElement(I->getOperand(0), laneIndex));
      arguments.push_back(
          Builder.CreateExtractElement(I->getOperand(1), laneIndex));
      if (opCode == FOP_FMA) {
        arguments.push_back(
            Builder.CreateExtractElement(I->getOperand(2), laneIndex));
      }

      Value *laneResult = Builder.CreateCall(scalarFunction, arguments);
      result = Builder.CreateInsertElement(result, laneResult, laneIndex);
    }

    if (opCode == FOP_CMP && result->getType() != I->getType())
      return Builder.CreateIntCast(result, I->getType(), true);
    return result;
  }

  bool hasExpectedByValABI(
      Function *F, const std::vector<std::pair<unsigned, Type *>> &byValParams,
      Module &M) {
    for (auto &[index, byValType] : byValParams) {
      Attribute byValAttr =
          F->getParamAttribute(index, Attribute::AttrKind::ByVal);
      if (!byValAttr.isValid() || byValAttr.getValueAsType() != byValType)
        return false;

      Attribute alignAttr =
          F->getParamAttribute(index, Attribute::AttrKind::Alignment);
      Align expectedAlign = M.getDataLayout().getABITypeAlign(byValType);
      if (!alignAttr.isValid() ||
          alignAttr.getValueAsInt() != expectedAlign.value())
        return false;
    }
    return true;
  }

  Function *createMCAFunctionDeclaration(Module &M, const std::string &name,
                                         Instruction *I, FPOps opCode,
                                         StringRef isaSuffix) {
    std::vector<Type *> paramTypes;
    std::vector<std::pair<unsigned, Type *>> byValParams;

    if (opCode == FOP_CMP) {
      paramTypes.push_back(Type::getInt32Ty(M.getContext()));
      addParameter(I->getOperand(0)->getType(), paramTypes, byValParams, M,
                   isaSuffix);
      addParameter(I->getOperand(1)->getType(), paramTypes, byValParams, M,
                   isaSuffix);
    } else if (opCode == FOP_FMA) {
      addParameter(I->getOperand(0)->getType(), paramTypes, byValParams, M,
                   isaSuffix);
      addParameter(I->getOperand(1)->getType(), paramTypes, byValParams, M,
                   isaSuffix);
      addParameter(I->getOperand(2)->getType(), paramTypes, byValParams, M,
                   isaSuffix);
    } else if (opCode == FOP_CAST) {
      addParameter(I->getOperand(0)->getType(), paramTypes, byValParams, M,
                   isaSuffix);
    } else {
      addParameter(I->getOperand(0)->getType(), paramTypes, byValParams, M,
                   isaSuffix);
      addParameter(I->getOperand(1)->getType(), paramTypes, byValParams, M,
                   isaSuffix);
    }

    Type *returnType = opCode == FOP_CMP ? getComparisonResultType(I, M)
                                         : getABIType(I->getType(), M);
    FunctionType *functionType =
        FunctionType::get(returnType, paramTypes, false);

    if (Function *F = M.getFunction(name)) {
      if (F->getFunctionType() == functionType &&
          hasExpectedByValABI(F, byValParams, M)) {
        addTargetAttributes(F, isaSuffix);
        return F;
      }
      if (!F->isDeclaration()) {
        errs() << "Conflicting MCA helper definition for " << name << "\n";
        report_fatal_error("libVFCInstrument fatal error");
      }
      F->eraseFromParent();
    }

    auto callee = M.getOrInsertFunction(name, functionType);
    auto *F = cast<Function>(callee.getCallee());

    for (auto &[index, byValType] : byValParams) {
      F->addParamAttr(index,
                      Attribute::getWithByValType(M.getContext(), byValType));
      F->addParamAttr(index, Attribute::getWithAlignment(
                                 M.getContext(),
                                 M.getDataLayout().getABITypeAlign(byValType)));
    }

    addTargetAttributes(F, isaSuffix);
    return F;
  }

  Value *replaceWithMCACall(Module &M, Instruction *I, FPOps opCode) {
    if (not isValidInstruction(I)) {
      return nullptr;
    }

    IRBuilder<> Builder(I);

    if (shouldScalarizeVectorReturn(M, I, opCode))
      return replaceWithScalarizedMCACalls(M, Builder, I, opCode);

    Function *caller = I->getFunction();
    /* Get the mca function */
    Function *mcaFunction = getMCAFunction(M, I, opCode);
    if (mcaFunction == nullptr)
      return nullptr;

    // If the caller and the callee (mcaFunction) have different ABI we set the
    // caller attributes to the callee ones.
    // Guard with isValid(): getFnAttribute returns a null Attribute when the
    // attribute is absent, and addFnAttr on an invalid Attribute corrupts the
    // IR and crashes the LLVM verifier.
    if (areFunctionArgsABICompatible(caller, mcaFunction)) {
      auto target_features = mcaFunction->getFnAttribute("target-features");
      auto target_cpu = mcaFunction->getFnAttribute("target-cpu");
      if (target_features.isValid())
        caller->addFnAttr(target_features);
      if (target_cpu.isValid())
        caller->addFnAttr(target_cpu);
    }

    // We call directly a hardcoded helper function
    // no need to go through the vtable at this stage.
    Value *newInst;
    if (opCode == FOP_CMP) {
      newInst = replaceComparisonWithMCACall(Builder, mcaFunction, I);
    } else if (opCode == FOP_FMA) {
      newInst = replaceArithmeticFMAWithMCACall(Builder, mcaFunction, I);
    } else if (opCode == FOP_CAST) {
      newInst = replaceCastWithMCACall(Builder, mcaFunction, I);
    } else {
      newInst = replaceArithmeticWithMCACall(Builder, mcaFunction, I);
    }
    return newInst;
  }

  bool isFMAOperation(Instruction &I) {
    CallInst *CI = static_cast<CallInst *>(&I);
    if (CI->getCalledFunction() == nullptr)
      return false;

    const auto name = CI->getCalledFunction()->getName();
    if (name.empty())
      return false;
    if (name == "llvm.fmuladd.f32")
      return true;
    if (name == "llvm.fmuladd.f64")
      return true;
    if (name == "llvm.fma.f32")
      return true;
    if (name == "llvm.fma.f64")
      return true;
    return false;
  }

  /* Check if the cast operation is valid */
  /* for now, we only support a cast from double to float */
  bool isValidCastOperation(Instruction &I) {
    FPTruncInst *FPTrunc = static_cast<FPTruncInst *>(&I);
    Type *srcType = FPTrunc->getSrcTy();
    Type *dstType = FPTrunc->getDestTy();
    if (srcType->isDoubleTy() and dstType->isFloatTy()) {
      return true;
    }

    return false;
  }

  FPOps mustReplace(Instruction &I) {
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
    case Instruction::Call:
      // Only instrument FMA if the flag --inst-fma is passed
      if (VfclibInstInstrumentFMA and isFMAOperation(I)) {
        return FOP_FMA;
      } else {
        return FOP_IGNORE;
      }
    case Instruction::FPTrunc:
      // Only instrument cast if the flag --inst-cast is passed
      if (VfclibInstInstrumentCast and isValidCastOperation(I)) {
        return FOP_CAST;
      } else {
        return FOP_IGNORE;
      }
    default:
      return FOP_IGNORE;
    }
  }

  bool runOnBasicBlock(Module &M, BasicBlock &B) {
    bool modified = false;
    std::set<std::pair<Instruction *, FPOps>> WorkList;
    for (BasicBlock::iterator ii = B.begin(), ie = B.end(); ii != ie; ++ii) {
      Instruction &I = *ii;
      FPOps opCode = mustReplace(I);
      if (opCode == FOP_IGNORE)
        continue;
      WorkList.insert(std::make_pair(&I, opCode));
    }

    for (auto p : WorkList) {
      Instruction *I = p.first;
      FPOps opCode = p.second;
      if (VfclibInstVerbose)
        errs() << "Instrumenting" << *I << '\n';
      Value *value = replaceWithMCACall(M, I, opCode);
      if (value != nullptr) {
        BasicBlock::iterator ii(I);
        ReplaceInstWithValue(ii, value);
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
