add_llvm_component_group(TestXPU)

set(LLVM_TARGET_DEFINITIONS TestXPU.td)

tablegen(LLVM TestXPUGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM TestXPUGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM TestXPUGenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(TestXPUCommonTableGen)

add_llvm_target(TestXPUCodeGen
  TestXPUTargetMachine.cpp

  # LINK_COMPONENTS
  #  Analysis
  # AsmPrinter
  # CodeGen
  # Core
  # TestXPUAsmParser
  # TestXPUDesc
  # TestXPUInfo
  # IPO
  # MC
  # Scalar
  # SelectionDAG
  # Support
  # Target
  # TransformUtils

  ADD_TO_COMPONENT
  TestXPU
)

#add_subdirectory(MCTargetDesc)
#add_subdirectory(TargetInfo)

