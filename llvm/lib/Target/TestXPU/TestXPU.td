//===----------------------------------------------------------------------===//
// Target-independent interfaces
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

//===----------------------------------------------------------------------===//
// Target-dependent interfaces
//===----------------------------------------------------------------------===//

// Register Infomation
include "TestXPURegisterInfo.td"

// Calling Conversion
//include "TestXPUSchedule.td"

// Instruction Description
include "TestXPUInstrInfo.td"

def TestXPUInstrInfo : InstrInfo;

def TestXPU : Target {
  // def TestXPUInstrInfo : InstrInfo as before.
  let InstructionSet = TestXPUInstrInfo;
}
