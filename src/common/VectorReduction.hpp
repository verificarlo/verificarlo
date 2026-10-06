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
#ifndef VERIFICARLO_COMMON_VECTOR_REDUCTION_HPP
#define VERIFICARLO_COMMON_VECTOR_REDUCTION_HPP

#include <vector>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"

namespace vfc {

/* llvm.vector.reduce.fadd/fmul(start, <N x T> v) computes
 *   (((start op v[0]) op v[1]) ... op v[N-1])
 * in that order unless the call carries the reassoc flag. The loop vectorizer
 * emits these intrinsics for floating-point sum and product loops: with
 * -ffast-math on every target, and even without it on AArch64, where strict
 * (ordered) reductions are vectorized. The arithmetic they perform would
 * otherwise escape instrumentation, so rewrite each fixed-width float/double
 * reduction in B as the equivalent chain of scalar fadd/fmul, which the
 * instrumentation passes then handle like any other operation. Reductions
 * on scalable vectors are left as is. Returns true if B was modified. */
inline bool expandVectorReductions(llvm::BasicBlock &B) {
  using namespace llvm;

  std::vector<IntrinsicInst *> reductions;
  for (auto &I : B) {
    auto *II = dyn_cast<IntrinsicInst>(&I);
    if (II == nullptr)
      continue;
    const auto id = II->getIntrinsicID();
    if (id != Intrinsic::vector_reduce_fadd and
        id != Intrinsic::vector_reduce_fmul)
      continue;
    auto *vecTy = dyn_cast<FixedVectorType>(II->getArgOperand(1)->getType());
    if (vecTy == nullptr)
      continue;
    auto *eltTy = vecTy->getElementType();
    if (not eltTy->isFloatTy() and not eltTy->isDoubleTy())
      continue;
    reductions.push_back(II);
  }

  for (auto *II : reductions) {
    IRBuilder<> Builder(II);
    Builder.setFastMathFlags(II->getFastMathFlags());
    const bool isAdd = II->getIntrinsicID() == Intrinsic::vector_reduce_fadd;
    Value *acc = II->getArgOperand(0);
    Value *vec = II->getArgOperand(1);
    const auto lanes = cast<FixedVectorType>(vec->getType())->getNumElements();
    for (unsigned lane = 0; lane < lanes; lane++) {
      Value *elt = Builder.CreateExtractElement(vec, Builder.getInt64(lane));
      acc = isAdd ? Builder.CreateFAdd(acc, elt) : Builder.CreateFMul(acc, elt);
    }
    II->replaceAllUsesWith(acc);
    II->eraseFromParent();
  }

  return not reductions.empty();
}

} // namespace vfc

#endif // VERIFICARLO_COMMON_VECTOR_REDUCTION_HPP
