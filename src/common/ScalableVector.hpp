/*****************************************************************************\
 *                                                                           *\
 *  This file is part of the Verificarlo project,                            *\
 *  under the Apache License v2.0 with LLVM Exceptions.                      *\
 *  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception.                 *\
 *  See https://llvm.org/LICENSE.txt for license information.                *\
 *                                                                           *\
 *  Copyright (c) 2019-2026                                                  *\
 *     Verificarlo Contributors                                              *\
 *                                                                           *\
 ****************************************************************************/
#ifndef VERIFICARLO_COMMON_SCALABLE_VECTOR_HPP
#define VERIFICARLO_COMMON_SCALABLE_VECTOR_HPP

#include <optional>

#include "llvm/ADT/STLFunctionalExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"

/* Scalable vectors (<vscale x N x T>, e.g. SVE on AArch64) have a number of
 * lanes known only at run time, so the passes cannot call fixed-width vector
 * wrappers for them. Instead, each operation on scalable vectors is replaced
 * by a loop over its lanes that calls the scalar instrumentation function on
 * each lane. */
namespace vfc {

enum class ScalableOpKind { Add, Sub, Mul, Div, Fma, Cmp };

/* A floating-point operation on scalable vectors. Each active lane i computes
 *   kind(±operands[0][i], ±operands[1][i], ...)
 * where operand k is negated when negate[k] is set (exact, so it does not
 * need instrumentation). When predicate is set, lanes whose predicate bit is
 * clear are not computed and take the value of passthru instead. The
 * operands are only valid until the IR is modified: replacing an operation
 * may erase an operand of another one, so match it again before replacing
 * it. */
struct ScalableOp {
  llvm::Instruction *I = nullptr;
  ScalableOpKind kind = ScalableOpKind::Add;
  llvm::SmallVector<llvm::Value *, 3> operands;
  llvm::SmallVector<bool, 3> negate;
  llvm::CmpInst::Predicate cmpPredicate = llvm::CmpInst::BAD_FCMP_PREDICATE;
  llvm::Value *predicate = nullptr;
  llvm::Value *passthru = nullptr;
};

inline bool isScalableFPVector(llvm::Type *Ty) {
  auto *vecTy = llvm::dyn_cast<llvm::ScalableVectorType>(Ty);
  if (vecTy == nullptr)
    return false;
  auto *eltTy = vecTy->getElementType();
  return eltTy->isFloatTy() or eltTy->isDoubleTy();
}

namespace detail {

/* Predicated SVE arithmetic intrinsics, llvm.aarch64.sve.<name>[.u].<type>,
 * with arguments (pg, x, y[, z]). Inactive lanes take x for the merging
 * form; they are undefined for the .u form, so x is fine as well. operands
 * lists the argument indices (1-based after pg) in the order expected by
 * kind, e.g. fma(a, b, c) = a * b + c. */
struct SVEIntrinsicInfo {
  llvm::StringRef name;
  ScalableOpKind kind;
  unsigned operands[3];
  bool negate[3];
};

// clang-format off
const SVEIntrinsicInfo sveIntrinsics[] = {
    {"fadd",  ScalableOpKind::Add, {1, 2, 0}, {false, false, false}},
    {"fsub",  ScalableOpKind::Sub, {1, 2, 0}, {false, false, false}},
    {"fsubr", ScalableOpKind::Sub, {2, 1, 0}, {false, false, false}},
    {"fmul",  ScalableOpKind::Mul, {1, 2, 0}, {false, false, false}},
    {"fdiv",  ScalableOpKind::Div, {1, 2, 0}, {false, false, false}},
    {"fdivr", ScalableOpKind::Div, {2, 1, 0}, {false, false, false}},
    // (pg, acc, a, b): fmla = acc + a*b, fmls = acc - a*b,
    //                  fnmla = -acc - a*b, fnmls = -acc + a*b
    {"fmla",  ScalableOpKind::Fma, {2, 3, 1}, {false, false, false}},
    {"fmls",  ScalableOpKind::Fma, {2, 3, 1}, {true,  false, false}},
    {"fnmla", ScalableOpKind::Fma, {2, 3, 1}, {true,  false, true}},
    {"fnmls", ScalableOpKind::Fma, {2, 3, 1}, {false, false, true}},
    // (pg, a, b, c): fmad = a*b + c, fmsb = c - a*b,
    //                fnmad = -a*b - c, fnmsb = a*b - c
    {"fmad",  ScalableOpKind::Fma, {1, 2, 3}, {false, false, false}},
    {"fmsb",  ScalableOpKind::Fma, {1, 2, 3}, {true,  false, false}},
    {"fnmad", ScalableOpKind::Fma, {1, 2, 3}, {true,  false, true}},
    {"fnmsb", ScalableOpKind::Fma, {1, 2, 3}, {false, false, true}},
};
// clang-format on

inline unsigned getArity(ScalableOpKind kind) {
  return kind == ScalableOpKind::Fma ? 3 : 2;
}

inline std::optional<ScalableOp> matchSVEIntrinsic(llvm::CallInst *CI) {
  using namespace llvm;
  Function *callee = CI->getCalledFunction();
  if (callee == nullptr or not isScalableFPVector(CI->getType()))
    return std::nullopt;

  // llvm.aarch64.sve.<name>[.u].<type suffix>
  StringRef name = callee->getName();
  if (not name.consume_front("llvm.aarch64.sve."))
    return std::nullopt;
  // The .u form (undefined inactive lanes) is handled like the merging form.
  // Other variants (e.g. fmul.lane) have different operands.
  auto [op, rest] = name.split('.');
  rest.consume_front("u.");
  if (not rest.consume_front("nxv"))
    return std::nullopt;

  for (const auto &info : sveIntrinsics) {
    if (op != info.name)
      continue;
    unsigned arity = getArity(info.kind);
    if (CI->arg_size() != arity + 1 or
        not CI->getArgOperand(0)->getType()->isIntOrIntVectorTy(1))
      return std::nullopt;
    ScalableOp result;
    result.I = CI;
    result.kind = info.kind;
    for (unsigned k = 0; k < arity; k++) {
      result.operands.push_back(CI->getArgOperand(info.operands[k]));
      result.negate.push_back(info.negate[k]);
    }
    result.predicate = CI->getArgOperand(0);
    result.passthru = CI->getArgOperand(1);
    return result;
  }
  return std::nullopt;
}

} // namespace detail

/* Matches a floating-point operation on scalable vectors of float or double:
 * fadd/fsub/fmul/fdiv, fcmp, llvm.fma/llvm.fmuladd and the predicated SVE
 * arithmetic intrinsics. */
inline std::optional<ScalableOp> matchScalableOp(llvm::Instruction &I) {
  using namespace llvm;

  if (auto *FCI = dyn_cast<FCmpInst>(&I)) {
    if (not isScalableFPVector(FCI->getOperand(0)->getType()))
      return std::nullopt;
    ScalableOp result;
    result.I = &I;
    result.kind = ScalableOpKind::Cmp;
    result.operands = {FCI->getOperand(0), FCI->getOperand(1)};
    result.negate = {false, false};
    result.cmpPredicate = FCI->getPredicate();
    return result;
  }

  if (not isScalableFPVector(I.getType()))
    return std::nullopt;

  std::optional<ScalableOpKind> kind;
  switch (I.getOpcode()) {
  case Instruction::FAdd:
    kind = ScalableOpKind::Add;
    break;
  case Instruction::FSub:
    kind = ScalableOpKind::Sub;
    break;
  case Instruction::FMul:
    kind = ScalableOpKind::Mul;
    break;
  case Instruction::FDiv:
    kind = ScalableOpKind::Div;
    break;
  case Instruction::Call: {
    auto *CI = cast<CallInst>(&I);
    auto id = CI->getIntrinsicID();
    if (id == Intrinsic::fma or id == Intrinsic::fmuladd) {
      kind = ScalableOpKind::Fma;
      break;
    }
    return detail::matchSVEIntrinsic(CI);
  }
  default:
    return std::nullopt;
  }

  ScalableOp result;
  result.I = &I;
  result.kind = *kind;
  for (unsigned k = 0; k < detail::getArity(*kind); k++) {
    result.operands.push_back(I.getOperand(k));
    result.negate.push_back(false);
  }
  return result;
}

/* Emits before I the loop
 *   acc = init
 *   for (lane = 0; lane < lanes; lane++)
 *     if (predicate == nullptr or predicate[lane])
 *       acc = step(Builder, lane, acc)
 * and returns the final value of acc, which dominates I. lanes is at least
 * 1 (vscale >= 1). The block of I is split, so I and the instructions after
 * it move to a new block. */
inline llvm::Value *
emitLaneLoop(llvm::Instruction *I, uint64_t minLanes, llvm::Value *init,
             llvm::Value *predicate,
             llvm::function_ref<llvm::Value *(llvm::IRBuilder<> &,
                                              llvm::Value *, llvm::Value *)>
                 step) {
  using namespace llvm;

  BasicBlock *pre = I->getParent();
  Function *F = pre->getParent();
  LLVMContext &C = F->getContext();
  BasicBlock *exit = pre->splitBasicBlock(I, "vfc.lanes.exit");
  pre->getTerminator()->eraseFromParent();

  BasicBlock *header = BasicBlock::Create(C, "vfc.lanes", F, exit);
  BasicBlock *body = header;
  BasicBlock *latch = header;
  if (predicate != nullptr) {
    body = BasicBlock::Create(C, "vfc.lanes.active", F, exit);
    latch = BasicBlock::Create(C, "vfc.lanes.next", F, exit);
  }

  IRBuilder<> Builder(pre);
  Builder.SetCurrentDebugLocation(I->getDebugLoc());
  Value *lanes = Builder.CreateVScale(Builder.getInt64(minLanes));
  Builder.CreateBr(header);

  Builder.SetInsertPoint(header);
  PHINode *lane = Builder.CreatePHI(Builder.getInt64Ty(), 2, "vfc.lane");
  PHINode *acc = Builder.CreatePHI(init->getType(), 2, "vfc.acc");
  lane->addIncoming(Builder.getInt64(0), pre);
  acc->addIncoming(init, pre);
  if (predicate != nullptr) {
    Value *active = Builder.CreateExtractElement(predicate, lane);
    Builder.CreateCondBr(active, body, latch);
    Builder.SetInsertPoint(body);
  }

  Value *next = step(Builder, lane, acc);

  if (predicate != nullptr) {
    Builder.CreateBr(latch);
    Builder.SetInsertPoint(latch);
    PHINode *merged = Builder.CreatePHI(init->getType(), 2);
    merged->addIncoming(acc, header);
    merged->addIncoming(next, body);
    next = merged;
  }
  Value *nextLane = Builder.CreateAdd(lane, Builder.getInt64(1));
  Builder.CreateCondBr(Builder.CreateICmpULT(nextLane, lanes), header, exit);
  lane->addIncoming(nextLane, latch);
  acc->addIncoming(next, latch);
  return next;
}

/* Replaces op.I by a loop calling emitLane on the scalar operands of each
 * active lane (see ScalableOp). emitLane returns the scalar result, which is
 * cast to the element type of the result if needed (e.g. the i32 result of
 * a comparison to i1). */
inline void replaceScalableOp(
    const ScalableOp &op,
    llvm::function_ref<llvm::Value *(llvm::IRBuilder<> &,
                                     llvm::ArrayRef<llvm::Value *>)>
        emitLane) {
  using namespace llvm;

  Instruction *I = op.I;
  auto *resultTy = cast<ScalableVectorType>(I->getType());
  Value *init = op.passthru ? op.passthru : PoisonValue::get(resultTy);

  Value *result = emitLaneLoop(
      I, resultTy->getMinNumElements(), init, op.predicate,
      [&](IRBuilder<> &Builder, Value *lane, Value *acc) -> Value * {
        SmallVector<Value *, 3> scalars;
        for (unsigned k = 0; k < op.operands.size(); k++) {
          Value *scalar = Builder.CreateExtractElement(op.operands[k], lane);
          if (op.negate[k])
            scalar = Builder.CreateFNeg(scalar);
          scalars.push_back(scalar);
        }
        Value *scalarResult = emitLane(Builder, scalars);
        Type *eltTy = resultTy->getElementType();
        if (scalarResult->getType() != eltTy)
          scalarResult = Builder.CreateIntCast(scalarResult, eltTy, true);
        return Builder.CreateInsertElement(acc, scalarResult, lane);
      });

  I->replaceAllUsesWith(result);
  I->eraseFromParent();
}

} // namespace vfc

#endif // VERIFICARLO_COMMON_SCALABLE_VECTOR_HPP
