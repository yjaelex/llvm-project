//===-- TestXPUSchedule.td - TestXPU Scheduling Definitions ------*- tablegen -*-===//
//
//                    The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.

//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Functional units across chips sets. Based on GCC/TestXPU backend files
//===----------------------------------------------------------------------===//
def ALU     : FuncUnit;
def IMULDIV : FuncUnit;

//===----------------------------------------------------------------------===//
// Instruction Itinerary classes used for TestXPU
//===----------------------------------------------------------------------===//
def IIAlu              : InstrItinClass;
def IICLO              : InstrItinClass;
def IICLZ              : InstrItinClass;
def IILoad             : InstrItinClass;
def IIStore            : InstrItinClass;
def IIBranch           : InstrItinClass;
def IIPseudo           : InstrItinClass;

//===----------------------------------------------------------------------===//
// TestXPU Generic instruction itineraries
//===----------------------------------------------------------------------===//
def TestXPUGenericItineraries : ProcessorItineraries<[ALU, IMULDIV], [], [
  InstrItinData<IIAlu               ,  [InstrStage<1,   [ALU]>]>,
  InstrItinData<IICLO               ,  [InstrStage<1,   [ALU]>]>,
  InstrItinData<IICLZ               ,  [InstrStage<1,   [ALU]>]>,
  InstrItinData<IILoad              ,  [InstrStage<3,   [ALU]>]>,
  InstrItinData<IIStore             ,  [InstrStage<1,   [ALU]>]>,
  InstrItinData<IIBranch            ,  [InstrStage<1,   [ALU]>]>
]>;
