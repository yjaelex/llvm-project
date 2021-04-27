add_llvm_component_library(LLVMTestXPUInfo
  TestXPUTargetInfo.cpp

  LINK_COMPONENTS
  Support

  ADD_TO_COMPONENT
  TestXPU
  )
