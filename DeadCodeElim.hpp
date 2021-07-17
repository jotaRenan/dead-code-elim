#ifndef dead_code_elim
#define dead_code_elim

#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "range-analysis/RangeAnalysis.h"

class DeadCodeElim : public FunctionPass {
    static char ID;
    DeadCodeElim() : FunctionPass(ID) {}
    ~DeadCodeElim() {}
    bool runOnFunction(Function &Fun) override;
    void handle_compare(ICmpInst *ICM, InterProceduralRA<Cousot> &ra);
    void delete_path(Instruction *Inst);
}

#endif