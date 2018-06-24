#include "opcode.hxx"

#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace opcode {
  
  Fops getOpCode(const Instruction &I) {
    switch (I.getOpcode()) {
      case Instruction::FAdd:
        return Fops::FOP_ADD;
      case Instruction::FSub:
        // In LLVM IR the FSub instruction is used to represent FNeg
        return Fops::FOP_SUB;
      case Instruction::FMul:
        return Fops::FOP_MUL;
      case Instruction::FDiv:
        return Fops::FOP_DIV;
      case Instruction::Store:
        return Fops::STORE;
      case Instruction::Ret:
      	return Fops::RETURN;
    default:
        return Fops::FOP_IGNORE;
    }
  }

    Fops getOpCode(const Instruction *I) {
    switch (I->getOpcode()) {
      case Instruction::FAdd:
        return Fops::FOP_ADD;
      case Instruction::FSub:
        // In LLVM IR the FSub instruction is used to represent FNeg
        return Fops::FOP_SUB;
      case Instruction::FMul:
        return Fops::FOP_MUL;
      case Instruction::FDiv:
        return Fops::FOP_DIV;
      case Instruction::Store:
        return Fops::STORE;
      case Instruction::Ret:
      	return Fops::RETURN;
    default:
        return Fops::FOP_IGNORE;
    }
  }

  std::string getOpStr(const Instruction *I) {
    switch (I->getOpcode()) {
      case Instruction::FAdd:
        return "+";
      case Instruction::FSub:
        // In LLVM IR the FSub instruction is used to represent FNeg
        return "-";
      case Instruction::FMul:
        return "*";
      case Instruction::FDiv:
	return "/";
    case Instruction::Store:
        return "<-";
    default:
      return "";
    }
  }
  
  bool isFPOp(Fops op) {
    switch (op) {
    case Fops::FOP_ADD:
    case Fops::FOP_MUL:
    case Fops::FOP_SUB:
    case Fops::FOP_DIV:
      return true;
    default:
      return false;      
    }
  }

  bool isFPOp(Instruction &I) {
    switch (I.getOpcode()) {
    case Instruction::FAdd:
    case Instruction::FSub:
    case Instruction::FMul:
    case Instruction::FDiv:
      return true;
    default:
      return false;
    }
  }

  bool isFPOp(const Instruction *I) {
    switch (I->getOpcode()) {
    case Instruction::FAdd:
    case Instruction::FSub:
    case Instruction::FMul:
    case Instruction::FDiv:
      return true;
    default:
      return false;
    }
  }

  bool isStoreOp(Instruction &I) {
    return I.getOpcode() == Instruction::Store;
  }
  
  bool isStoreOp(const Instruction *I) {
    return I->getOpcode() == Instruction::Store;
  }

  bool isRetOp(Instruction &I) {
    return I.getOpcode() == Instruction::Ret;
  }

  bool isRetOp(const Instruction *I) {
    return I->getOpcode() == Instruction::Ret;
  }

  bool isIgnoreOp(const Instruction &I) {
    return opcode::getOpCode(I) == Fops::FOP_IGNORE;
  }

  bool isIgnoreOp(const Instruction *I) {
    return opcode::getOpCode(I) == Fops::FOP_IGNORE;
  }

  std::string fops_str(Fops op) {
    switch (op) {
    case Fops::FOP_ADD:
      return "FOP_ADD";
    case Fops::FOP_SUB:
      return "FOP_SUB";
    case Fops::FOP_MUL:
      return "FOP_MUL";
    case Fops::FOP_DIV:
      return "FOP_DIV";
    case Fops::STORE:
      return "STORE";
    case Fops::RETURN:
      return "RETURN";
    case Fops::FOP_IGNORE:
      return "IGNORE";
    default:
      llvm_unreachable("Bad Fops");
    }
  }
}
