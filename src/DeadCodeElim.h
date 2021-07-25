#ifndef dead_code_elim
#define dead_code_elim

#include <iostream>
#include <fstream>
#include <sstream>

#include "llvm/Pass.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/User.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Local.h"

#include "RangeAnalysis.h"

class DeadCodeElim : public FunctionPass {
    public:
        static char ID;
        DeadCodeElim() : FunctionPass(ID) {}
        ~DeadCodeElim() {}
        void getAnalysisUsage(AnalysisUsage &AU) const;
        bool runOnFunction(Function &Fun);

    private:
        void handle_compare(ICmpInst *ICM, InterProceduralRA<Cousot> &ra);
        void delete_path(Instruction *Inst, int indexTakenPath, int indexNotTakenPath);
        void merge_basic_blocks(BasicBlock *B1, BasicBlock *B2);
        void delete_basic_block(Instruction *Inst);
        void delete_instructions(BasicBlock *BB);

        bool has_unique_successor(BasicBlock *BB);
        void delete_false_path(Instruction *Inst);
        void delete_true_path(Instruction *Inst);
        BranchInst* cast_branch_instruction(Instruction *Inst);
};

#endif