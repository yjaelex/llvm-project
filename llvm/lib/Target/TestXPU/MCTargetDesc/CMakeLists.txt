add_llvm_component_library(LLVMTestXPUDesc
  TestXPUMCTargetDesc.cpp
  TestXPUMCAsmInfo.cpp

  LINK_COMPONENTS
  MC
  Support

  ADD_TO_COMPONENT
  TestXPU
  )

