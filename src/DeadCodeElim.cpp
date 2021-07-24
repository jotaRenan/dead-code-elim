#include <stdio.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include "llvm/Pass.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/User.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IRBuilder.h"

#include "RangeAnalysis.h"
#define DEBUG_TYPE "dce-range-anal"

using namespace llvm;
using namespace std;

STATISTIC(InstructionsEliminated, "Number of instructions eliminated");
STATISTIC(BasicBlocksEliminated,  "Number of basic blocks entirely eliminated");

namespace {
    class DeadCodeElim : public FunctionPass {
        public:
		static char ID;

        DeadCodeElim() : FunctionPass(ID) {}
        ~DeadCodeElim() {}

        BranchInst* cast_branch_instruction(Instruction *Inst) {
            return dyn_cast<BranchInst>(Inst);
        }

        void delete_true_path(Instruction *Inst) {
            /* para deletar o path em que a condicao eh verdadeira
               devemos apagar o primeiro sucessor da instrucao     */
            delete_path(Inst, 1, 0);
        }

        void delete_false_path(Instruction *Inst) {
            /* para deletar o path em que a condicao eh falsa
               devemos apagar o segundo sucessor da instrucao     */
            delete_path(Inst, 0, 1);
        }

        void delete_path(Instruction *Inst, int indexTakenPath, int indexNotTakenPath) {
            BranchInst *BInst = cast_branch_instruction(Inst->getNextNode());
            cout << "\nverificando delecao";
            if (!BInst) return;
            cout << "\nindo deletar";
            
            BasicBlock *ParentBB = BInst->getParent();
            BasicBlock *takenPath = BInst->getSuccessor(indexTakenPath);
            BasicBlock *notTakenPath = BInst->getSuccessor(indexNotTakenPath);
            
            notTakenPath->removePredecessor(ParentBB);
            BranchInst *newBranch = BranchInst::Create(takenPath);
            ReplaceInstWithInst(BInst, newBranch);

            delete_basic_block(notTakenPath->getTerminator());
            merge_basic_blocks(ParentBB, takenPath);
        }

        void merge_basic_blocks(BasicBlock *B1, BasicBlock *B2) {
            if (!has_unique_successor(B2)) return;
            
            BasicBlocksEliminated++;
            BasicBlock* B2Succ = B2->getUniqueSuccessor();
            
            B1->getTerminator()->eraseFromParent();
            IRBuilder<> Builder(B1);

            Builder.CreateBr(B2);
            MergeBlockIntoPredecessor(B2);

            merge_basic_blocks(B1, B2Succ);
        }

        bool has_single_predecessor(BasicBlock *BB) {
            return BB->getSinglePredecessor() != nullptr;
        }

        bool has_unique_successor(BasicBlock *BB) {
            return BB->getUniqueSuccessor() != nullptr;
        }

        void count_deleted_instructions_on_basic_block_removal(BasicBlock *BB) {
            InstructionsEliminated++; // instrucao de chamada ao basic block
            for (Instruction &Inst: *BB) {
	            InstructionsEliminated++;
            }
        }

        void delete_basic_block(Instruction *Inst) {
            BasicBlock *ParentBB = Inst->getParent();
            
            if (ParentBB->hasNPredecessors(0)) { 
                BranchInst *BInst = cast_branch_instruction(Inst);
                if (!BInst) return;
                
                BInst->eraseFromParent();
                // remove basic block que contem a condicao
                ParentBB->eraseFromParent(); 
                count_deleted_instructions_on_basic_block_removal(ParentBB);
                BasicBlocksEliminated++;

                for (BasicBlock *succ : successors(ParentBB)) {
                    delete_basic_block(succ->getTerminator());
                }
            }
        }


        bool runOnFunction(Function &Fun) override {
            InterProceduralRA<Cousot> &ra = getAnalysis<InterProceduralRA<Cousot>>();
            for (Function::iterator bb = Fun.begin(), bbEnd = Fun.end(); bb != bbEnd; ++bb) {
                for (BasicBlock::iterator Inst = bb->begin(), IEnd = bb->end(); Inst != IEnd; ++Inst) {
                    printf("\navaliando instrucao");
                    ICmpInst *I = dyn_cast<ICmpInst>(Inst);
                    if (I) {
                        printf("\ninstrucao eh de comparacao");
                        handle_compare(I, ra);
                    }
                }
            }
            printf("\n\nterminando os trabalhos\n\n");
            return true;
        }

        void handle_compare(ICmpInst *ICM, InterProceduralRA<Cousot> &ra) {
            Range r1 = ra.getRange(ICM->getOperand(0));
            Range r2 = ra.getRange(ICM->getOperand(1));
            switch (ICM->getPredicate()) {
                case CmpInst::ICMP_SLT:
                case CmpInst::ICMP_ULT:
                    // lidar com <
                    printf("\nLIDANDO COM <");
                    r1.print(errs());
                    r2.print(errs());
                    errs() <<"\n";
                    if (r1.getUpper().slt(r2.getLower())) { // sempre da true entao podemos 
                        printf("\nSEMPRE DA TRU");
                        // remover o false-branch
                        delete_false_path(ICM);
                    } else if (r1.getLower().sge(r2.getUpper())) { // sempre da false entao podemos
                        printf("\nSEMPRE DA FALS");
                        // remover o true-branch
                        delete_true_path(ICM);
                    } else {
                        printf("\nNEM SEMPRE DA FALS NEM SEMPRE DA TRU");
                    }
                    break;
                case CmpInst::ICMP_SLE:
                case CmpInst::ICMP_ULE:
                    //lidar com <=
                    // upper 1 <= lower 2
                    if (r1.getUpper().sle(r2.getLower())) {
                        delete_false_path(ICM);

                    } else if (r1.getLower().sgt(r2.getUpper())) {

                        delete_true_path(ICM);
                    }
                    break;
                
                case CmpInst::ICMP_SGE: 
                case CmpInst::ICMP_UGE: 
                    //lidar com >=
                    if (r1.getLower().sge(r2.getUpper())) { // sempre da true entao podemos 
                        // remover o false-branch
                        delete_false_path(ICM);
                    } else if (r1.getUpper().slt(r2.getLower())) { // codigo sempre dará false
                        // remover o true-branch 
                        delete_true_path(ICM);
                    }
                    break;
                    
                case CmpInst::ICMP_SGT: 
                case CmpInst::ICMP_UGT: 
                    //lidar com >
                    if (r1.getLower().sgt(r2.getUpper())) { // sempre da true entao podemos 
                        // remover o false-branch
                        delete_false_path(ICM);
                    } else if (r1.getUpper().sle(r2.getLower())) { // codigo sempre dará false
                        // tirar true-branch
                        delete_true_path(ICM);
                    }
                    break;
                case CmpInst::ICMP_NE: 
                    //lidar com !=
                    if (r1.getLower() == r1.getUpper() && r2.getLower() == r2.getUpper()) {
                        auto r1_val = r1.getLower();
                        auto r2_val = r2.getLower();
                        if (r1_val != r2_val) { //sempre chamará o true-branch
                            // remover false-branch
                            delete_false_path(ICM);
                        } else { // sempre chamará o false-branch
                            // remover true-branch
                            delete_true_path(ICM);
                        }
                    }
                    break;
                case CmpInst::ICMP_EQ: 
                    //lidar com ==
                    if (r1.getLower() == r1.getUpper() && r2.getLower() == r2.getUpper()) {
                        auto r1_val = r1.getLower();
                        auto r2_val = r2.getLower();
                        if (r1_val == r2_val) { //sempre chamará o true-branch
                            // remover false-branch
                            delete_false_path(ICM);
                        } else { // sempre chamará o false-branch
                            // remover true-branch
                            delete_true_path(ICM);
                        }
                    }
                    break;
                default:
                    printf("\nDefault switch case");
            }
        }
    
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
        AU.setPreservesAll();
        AU.addRequired<InterProceduralRA<Cousot>>();
    }
  };
}


char DeadCodeElim::ID = 0;
static RegisterPass<DeadCodeElim> X("deadCodeElim", "Remove dead code by using range analysis");
