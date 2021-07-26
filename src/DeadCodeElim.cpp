#include "DeadCodeElim.h"

#define DEBUG_TYPE "dead-code-elim"

using namespace llvm;
using namespace std;

STATISTIC(InstructionsEliminated, "Number of instructions eliminated");
STATISTIC(BasicBlocksEliminated,  "Number of basic blocks entirely eliminated");

bool DeadCodeElim::runOnFunction(Function &Fun) {
    InterProceduralRA<Cousot> &ra = getAnalysis<InterProceduralRA<Cousot>>();

    int initialBlocksCount, initialInstsCount;
    countBlocksAndInstructions(Fun, initialBlocksCount, initialInstsCount);
    for (Function::iterator bb = Fun.begin(), bbEnd = Fun.end(); 
            bb != bbEnd; ++bb) {
        for (BasicBlock::iterator Inst = bb->begin(), IEnd = bb->end(); 
                Inst != IEnd; ++Inst) {
            ICmpInst *I = dyn_cast<ICmpInst>(Inst);
            if (I) {
                handle_compare(I, ra);
            }
        }
    }

    int finalBlocksCount, finalInstsCount;
    countBlocksAndInstructions(Fun, finalBlocksCount, finalInstsCount);
    BasicBlocksEliminated = initialBlocksCount - finalBlocksCount;
    InstructionsEliminated = initialInstsCount - finalInstsCount;
    return true;
}

void DeadCodeElim::handle_compare(ICmpInst *ICM, InterProceduralRA<Cousot> &ra) {
    Range r1 = ra.getRange(ICM->getOperand(0));
    Range r2 = ra.getRange(ICM->getOperand(1));
    switch (ICM->getPredicate()) {
        // lidar com <

        // a primeira validação verifica se a comparação será sempre verdadeira
        // enquanto a segunda verifica se a comparação será sempre falsa
        case CmpInst::ICMP_SLT:
        case CmpInst::ICMP_ULT:
            if (r1.getUpper().slt(r2.getLower())) {  
                delete_false_path(ICM);
            } else if (r1.getLower().sge(r2.getUpper())) { 
                delete_true_path(ICM);
            }
            break;

        //lidar com <=
        case CmpInst::ICMP_SLE:
        case CmpInst::ICMP_ULE:
            if (r1.getUpper().sle(r2.getLower())) {
                delete_false_path(ICM);
            } else if (r1.getLower().sgt(r2.getUpper())) {
                delete_true_path(ICM);
            }
            break;

        //lidar com >=
        case CmpInst::ICMP_SGE: 
        case CmpInst::ICMP_UGE: 
            if (r1.getLower().sge(r2.getUpper())) {
                delete_false_path(ICM);
            } else if (r1.getUpper().slt(r2.getLower())) {
                delete_true_path(ICM);
            }
            break;
            
        //lidar com >
        case CmpInst::ICMP_SGT: 
        case CmpInst::ICMP_UGT: 
            if (r1.getLower().sgt(r2.getUpper())) {
                delete_false_path(ICM);
            } else if (r1.getUpper().sle(r2.getLower())) {
                delete_true_path(ICM);
            }
            break;

        //lidar com !=
        case CmpInst::ICMP_NE: 
            if (r1.getLower() == r1.getUpper() && 
                    r2.getLower() == r2.getUpper()) {
                auto r1_val = r1.getLower();
                auto r2_val = r2.getLower();
                if (r1_val != r2_val) {
                    delete_false_path(ICM);
                } else { 
                    delete_true_path(ICM);
                }
            }
            break;

        //lidar com ==
        case CmpInst::ICMP_EQ: 
            if (r1.getLower() == r1.getUpper() && 
                    r2.getLower() == r2.getUpper()) {
                auto r1_val = r1.getLower();
                auto r2_val = r2.getLower();
                if (r1_val == r2_val) {
                    delete_false_path(ICM);
                } else { 
                    delete_true_path(ICM);
                }
            }
            break;

        default:
            break;
    }
}

void DeadCodeElim::delete_basic_block(Instruction *Inst) {
    BasicBlock *BB = Inst->getParent();
    
    // apenas deleta o basic block se ele estiver morto
    // ou seja, não possuir nenhum predecessor
    if (BB->hasNPredecessors(0)) { 
        BranchInst *BInst = dyn_cast<BranchInst>(Inst);
        if (!BInst) return;

        // deleta referências futuras
        ConstantFoldTerminator(BB, true); 
        for (BasicBlock *succ : successors(BB)) {
            succ->removePredecessor(BB);
            delete_basic_block(succ->getTerminator());
        }
        
        // delta instruções
        delete_instructions(BB);
        BB->removeFromParent(); 
    }
}

void DeadCodeElim::merge_basic_blocks(BasicBlock *BB) {
    // se houver um único predecessor, mescla os basic blocks
    MergeBlockIntoPredecessor(BB);
    for (BasicBlock *succ : successors(BB)) {
        MergeBlockIntoPredecessor(succ);
    }
}

void DeadCodeElim::delete_path(Instruction *Inst, int indexTakenPath, 
        int indexNotTakenPath) {
    BranchInst *BInst = dyn_cast<BranchInst>(Inst->getNextNode());
    if (!BInst) return;
    
    BasicBlock *BB = BInst->getParent();
    BasicBlock *takenPath = BInst->getSuccessor(indexTakenPath);
    BasicBlock *notTakenPath = BInst->getSuccessor(indexNotTakenPath);

    // deleta o path do predecessor para o caminho não tomado
    notTakenPath->removePredecessor(BB);

    // cria uma nova referência de basic block devido a referências de memória
    BranchInst *newBranch = BranchInst::Create(takenPath);
    ReplaceInstWithInst(BInst, newBranch);

    delete_basic_block(notTakenPath->getTerminator());
    merge_basic_blocks(BB);
}

void DeadCodeElim::countBlocksAndInstructions(Function &Fun, int &blocks, 
        int &insts) {
    blocks = 0;
    insts = 0;
    for (Function::iterator bb = Fun.begin(), 
            bbEnd = Fun.end(); bb != bbEnd; ++bb) {
        blocks++;
        for (BasicBlock::iterator Inst = bb->begin(), 
                IEnd = bb->end(); Inst != IEnd; ++Inst) {
            insts++;
        }
    }
}

void DeadCodeElim::delete_true_path(Instruction *Inst) {
    /* para deletar o path em que a condição é verdadeira
        devemos apagar o primeiro sucessor da instrução     */
    delete_path(Inst, 1, 0);
}

void DeadCodeElim::delete_false_path(Instruction *Inst) {
    /* para deletar o path em que a condição é falsa
        devemos apagar o segundo sucessor da instrução     */
    delete_path(Inst, 0, 1);
}

void DeadCodeElim::delete_instructions(BasicBlock *BB) {
    while (!BB->empty()) {
        Instruction &Inst = BB->back();
        if (!Inst.use_empty()) {
            Inst.replaceAllUsesWith(UndefValue::get(Inst.getType()));
            Inst.dropAllReferences();
        }
        BB->getInstList().pop_back();
    }
}

void DeadCodeElim::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
    AU.addRequired<InterProceduralRA<Cousot>>();
}

char DeadCodeElim::ID = 0;
static RegisterPass<DeadCodeElim> X("deadCodeElim", 
    "Remove dead code by using range analysis");
