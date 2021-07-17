add_llvm_library( DeadCodeElim MODULE BUILDTREE_ONLY
  range-analysis/RangeAnalysis.cpp
  range-analysis/vSSA.cpp
  DeadCodeElim.cpp
  
  DEPENDS
  intrinsics_gen
  PLUGIN_TOOL
  opt
  )
