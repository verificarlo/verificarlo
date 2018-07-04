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

#ifndef OPCODE_HXX
#define OPCODE_HXX

#include "llvm/IR/Instructions.h"
#include <iostream>

namespace opcode {

enum class Fops {
  FOP_ADD,
  FOP_SUB,
  FOP_MUL,
  FOP_DIV,
  STORE,
  RETURN,
  FOP_IGNORE
};
std::string fops_str(Fops);
Fops getOpCode(const llvm::Instruction &I);
Fops getOpCode(const llvm::Instruction *I);
std::string getOpStr(const llvm::Instruction *I);
bool isFPOp(llvm::Instruction &I);
bool isFPOp(const llvm::Instruction *I);
bool isFPOp(Fops);
bool isStoreOp(llvm::Instruction &I);
bool isStoreOp(const llvm::Instruction *I);
bool isRetOp(llvm::Instruction &I);
bool isRetOp(const llvm::Instruction *I);
bool isIgnoreOp(llvm::Instruction &I);
bool isIgnoreOp(const llvm::Instruction *I);
bool isVectorOp(llvm::Instruction &I);
bool isVectorOp(const llvm::Instruction *I);
}

#endif /* OPCODE_HXX */
