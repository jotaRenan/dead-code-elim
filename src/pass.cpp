STATISTIC(InstructionsEliminated, "Number of instructions eliminated");
STATISTIC(BasicBlocksEliminated,  "Number of basic blocks entirely eliminated");

bool::RADeadCodeElimination::solveICmpInstruction(ICmpInst* I) {
  InterProceduralRA < Cousot >* ra = &getAnalysis < InterProceduralRA < Cousot > >();
  Range range1 = ra->getRange(I->getOperand(0));
  Range range2 = ra->getRange(I->getOperand(1));
  switch (I->getPredicate()){
    case CmpInst::ICMP_SLT:
    //This code is always true
    if (range1.getUpper().slt(range2.getLower())) {
      ...
    }
    break;
    case ...
  }
  ...
}
