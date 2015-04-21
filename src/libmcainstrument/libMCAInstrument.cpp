#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

namespace {
    struct McalibInst : public ModulePass {
        static char ID;

        McalibInst() : ModulePass(ID) {}


        bool runOnModule(Module &M)
        {
            for(Module::iterator F = M.begin(), E = M.end(); F!= E; ++F)
            {
                runOnFunction(M, *F);
            }
        }

        bool runOnFunction(Module &M, Function &F) {
            errs() << "In Function: ";
            errs().write_escaped(F.getName()) << '\n';

            for (Function::iterator bi = F.begin(), be = F.end(); bi != be; ++bi) {
                runOnBasicBlock(M, *bi);
            }
        }

        Instruction * replaceWithMCACall(Module &M, BasicBlock &B, Instruction &I, std::string opName) {
            Type * opType = I.getType();
            std::string opTypeName;


            std::string vectorName = "";
            Type * baseType = opType;
            if (opType->isVectorTy()) {
                VectorType * t = static_cast<VectorType*>(opType);
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
                                                       (Type*)0);

            IRBuilder<> builder(getGlobalContext());
            Instruction * newInst = builder.CreateCall2(
                cast<Function>(hookFunc),
                I.getOperand(0),
                I.getOperand(1),
                "");
            return newInst;
        }

        std::string mustReplace(Instruction &I) {
            switch(I.getOpcode()) {
            case Instruction::FAdd:
                return "add";
            case Instruction::FMul:
                return "mul";
            case Instruction::FDiv:
                return "div";
            default:
                return "";
            }
        }

        bool runOnBasicBlock(Module &M, BasicBlock &B) {
            for (BasicBlock::iterator ii = B.begin(), ie = B.end(); ii != ie; ++ii) {
                Instruction &I = *ii;
                std::string opName = mustReplace(I);
                if (opName == "") continue;
                errs() << "Instrumenting" << I << '\n';
                Instruction * newInst = replaceWithMCACall(M, B, I, opName);
                ReplaceInstWithInst(B.getInstList(), ii, newInst);
            }
        }
    };
}

char McalibInst::ID = 0;
static RegisterPass<McalibInst> X("mcalibinst", "mcalib pass", false, false);
