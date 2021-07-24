add_llvm_library( DeadCodeElim MODULE BUILDTREE_ONLY
  src/DeadCodeElim.cpp
  
  DEPENDS
  intrinsics_gen
  PLUGIN_TOOL
  opt
  )
