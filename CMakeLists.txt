add_llvm_library( DeadCodeElim MODULE BUILDTREE_ONLY
  src/RangeAnalysis.cpp
  src/DeadCodeElim.cpp
  src/vSSA.cpp
  
  DEPENDS
  intrinsics_gen
  PLUGIN_TOOL
  opt
  )
