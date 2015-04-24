#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

namespace {
    struct McalibInst : public ModulePass {
        static char ID;

        McalibInst() : ModulePass(ID) { }

        bool runOnModule(Module &M) {
            bool modified = false;
            for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
                modified |= runOnFunction(M, *F);
            }
            // runOnModule must return true if the pass modifies the IR
            return modified;
        }


        bool runOnFunction(Module &M, Function &F) {
            errs() << "In Function: ";
            errs().write_escaped(F.getName()) << '\n';

            bool modified = false;

            for (Function::iterator bi = F.begin(), be = F.end(); bi != be; ++bi) {
                modified |= runOnBasicBlock(M, *bi);
            }
            return modified;
        }

        Instruction *replaceWithMCACall(Module &M, BasicBlock &B, Instruction &I, std::string opName) {
            Type *opType = I.getType();
            std::string opTypeName;

            std::string vectorName = "";
            Type *baseType = opType;
            if (opType->isVectorTy()) {
                VectorType *t = static_cast<VectorType *>(opType);
                baseType = t->getElementType();
                unsigned size = t->getNumElements();

                if (size == 2) {
                    vectorName = "2x";
                } else if (size == 4) {
                    vectorName = "4x";
                } else {
                    errs() << "Unsuported vector size: " << size << "\n";
                    assert(0);
                }
            }

            if (baseType->isDoubleTy()) {
                opTypeName = vectorName + "double";
            } else if (baseType->isFloatTy()) {
                opTypeName = vectorName + "float";
            } else {
                errs() << "Unsupported operand type: " << *opType << "\n";
                assert(0);
            }

            std::string mcaFunctionName = "_" + opTypeName + opName;

            Constant *hookFunc = M.getOrInsertFunction(mcaFunctionName,
                                                       opType,
                                                       opType,
                                                       opType,
                                                       (Type *) 0);

            IRBuilder<> builder(getGlobalContext());
            Instruction *newInst = builder.CreateCall2(
                    cast<Function>(hookFunc),
                    I.getOperand(0),
                    I.getOperand(1),
                    "");
            return newInst;
        }

        std::string cmpOpCode(Instruction &I) {
            FCmpInst::Predicate p = (cast<FCmpInst>(I)).getPredicate();
            switch (p) {

                case CmpInst::FCMP_OEQ:
                case CmpInst::FCMP_UEQ:
                    return "eq";

                case CmpInst::FCMP_OGT:
                case CmpInst::FCMP_UGT:
                    return "gt";

                case CmpInst::FCMP_OGE:
                case CmpInst::FCMP_UGE:
                    return "ge";

                case CmpInst::FCMP_OLT:
                case CmpInst::FCMP_ULT:
                    return "lt";

                case CmpInst::FCMP_OLE:
                case CmpInst::FCMP_ULE:
                    return "le";

                case CmpInst::FCMP_ONE:
                case CmpInst::FCMP_UNE:
                    return "ne";

                default:
                    return "";
            }
        }

        std::string mustReplace(Instruction &I) {
            switch (I.getOpcode()) {
                case Instruction::FAdd:
                    return "add";
                case Instruction::FSub:
                    // In LLVM IR the FSub instruction is used to represent FNeg
                    return "sub";
                case Instruction::FMul:
                    return "mul";
                case Instruction::FDiv:
                    return "div";
                case Instruction::FCmp:
                    return cmpOpCode(I);
                default:
                    return "";
            }
        }

        bool runOnBasicBlock(Module &M, BasicBlock &B) {
            bool modified = false;
            for (BasicBlock::iterator ii = B.begin(), ie = B.end(); ii != ie; ++ii) {
                Instruction &I = *ii;
                std::string opName = mustReplace(I);
                if (opName == "") continue;
                errs() << "Instrumenting" << I << '\n';
                Instruction *newInst = replaceWithMCACall(M, B, I, opName);
                ReplaceInstWithInst(B.getInstList(), ii, newInst);
                modified = true;
            }
            return modified;
        }
    };
}

char McalibInst::ID = 0;
static RegisterPass<McalibInst> X("mcalibinst", "mcalib pass", false, false);
