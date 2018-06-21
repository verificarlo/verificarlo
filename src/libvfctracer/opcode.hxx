#ifndef OPCODE_HXX
#define OPCODE_HXX

#include "llvm/IR/Instructions.h"
#include <iostream>

namespace opcode {

  enum class Fops {FOP_ADD, FOP_SUB, FOP_MUL, FOP_DIV, STORE, RETURN, FOP_IGNORE};
  std::string fops_str(Fops);
  Fops getOpCode(const llvm::Instruction &I);  
  Fops getOpCode(const llvm::Instruction *I) ;  
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

}

#endif /* OPCODE_HXX */
