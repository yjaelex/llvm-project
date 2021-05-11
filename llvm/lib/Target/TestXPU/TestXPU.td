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

def FeatureTestXPU : SubtargetFeature<"testxpu", "TestXPUArchVersion", 
	"TestXPU", "TestXPU ISA Support All Feature">;

class Proc<string Name, list<SubtargetFeature> Features> :
    Processor<Name, TestXPUGenericItineraries, Features>;

def : Proc<"testxpu", [FeatureTestXPU]>; // enable the command paramters "-mcpu=testxpu"


def TestXPUInstrInfo : InstrInfo;

def TestXPU : Target {
  // def TestXPUInstrInfo : InstrInfo as before.
  let InstructionSet = TestXPUInstrInfo;
}
