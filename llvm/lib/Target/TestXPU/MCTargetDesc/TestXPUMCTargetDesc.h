//===-- TestXPUMCTargetDesc.h - TestXPU Target Descriptions -----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file provides TestXPU specific target descriptions.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_TestXPU_MCTARGETDESC_TestXPUMCTARGETDESC_H
#define LLVM_LIB_TARGET_TestXPU_MCTARGETDESC_TestXPUMCTARGETDESC_H

#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/Support/CommandLine.h"
#include <cstdint>
#include <string>

#define TestXPU_POINTER_SIZE 4

#define TestXPU_PointerSize (TestXPU_POINTER_SIZE)
#define TestXPU_PointerSize_Bits (TestXPU_POINTER_SIZE * 8)
#define TestXPU_WordSize TestXPU_PointerSize
#define TestXPU_WordSize_Bits TestXPU_PointerSize_Bits

// allocframe saves LR and FP on stack before allocating
// a new stack frame. This takes 8 bytes.
#define TestXPU_LRFP_SIZE 8

// Normal instruction size (in bytes).
#define TestXPU_INSTR_SIZE 4

// Maximum number of words and instructions in a packet.
#define TestXPU_PACKET_SIZE 4
#define TestXPU_MAX_PACKET_SIZE (TestXPU_PACKET_SIZE * TestXPU_INSTR_SIZE)
// Minimum number of instructions in an end-loop packet.
#define TestXPU_PACKET_INNER_SIZE 2
#define TestXPU_PACKET_OUTER_SIZE 3
// Maximum number of instructions in a packet before shuffling,
// including a compound one or a duplex or an extender.
#define TestXPU_PRESHUFFLE_PACKET_SIZE (TestXPU_PACKET_SIZE + 3)

// Name of the global offset table as defined by the TestXPU ABI
#define TestXPU_GOT_SYM_NAME "_GLOBAL_OFFSET_TABLE_"

namespace llvm {

struct InstrStage;
class FeatureBitset;
class MCAsmBackend;
class MCCodeEmitter;
class MCContext;
class MCInstrInfo;
class MCObjectTargetWriter;
class MCRegisterInfo;
class MCSubtargetInfo;
class MCTargetOptions;
class Target;
class Triple;
class StringRef;

extern cl::opt<bool> TestXPUDisableCompound;
extern cl::opt<bool> TestXPUDisableDuplex;
extern const InstrStage TestXPUStages[];

MCInstrInfo *createTestXPUMCInstrInfo();
MCRegisterInfo *createTestXPUMCRegisterInfo(StringRef TT);

namespace TestXPU_MC {
  StringRef selectTestXPUCPU(StringRef CPU);

  FeatureBitset completeHVXFeatures(const FeatureBitset &FB);
  /// Create a TestXPU MCSubtargetInfo instance. This is exposed so Asm parser,
  /// etc. do not need to go through TargetRegistry.
  MCSubtargetInfo *createTestXPUMCSubtargetInfo(const Triple &TT, StringRef CPU,
                                                StringRef FS);
  MCSubtargetInfo const *getArchSubtarget(MCSubtargetInfo const *STI);
  void addArchSubtarget(MCSubtargetInfo const *STI,
                        StringRef FS);
  unsigned GetELFFlags(const MCSubtargetInfo &STI);

  llvm::ArrayRef<MCPhysReg> GetVectRegRev();
}

MCCodeEmitter *createTestXPUMCCodeEmitter(const MCInstrInfo &MCII,
                                          const MCRegisterInfo &MRI,
                                          MCContext &MCT);

MCAsmBackend *createTestXPUAsmBackend(const Target &T,
                                      const MCSubtargetInfo &STI,
                                      const MCRegisterInfo &MRI,
                                      const MCTargetOptions &Options);

std::unique_ptr<MCObjectTargetWriter>
createTestXPUELFObjectWriter(uint8_t OSABI, StringRef CPU);

unsigned TestXPUGetLastSlot();
unsigned TestXPUConvertUnits(unsigned ItinUnits, unsigned *Lanes);

} // End llvm namespace

// Define symbolic names for TestXPU registers.  This defines a mapping from
// register name to register number.
//
#define GET_REGINFO_ENUM
#include "TestXPUGenRegisterInfo.inc"

// Defines symbolic names for the TestXPU instructions.
//
#define GET_INSTRINFO_ENUM
#define GET_INSTRINFO_SCHED_ENUM
#include "TestXPUGenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "TestXPUGenSubtargetInfo.inc"

#endif // LLVM_LIB_TARGET_TestXPU_MCTARGETDESC_TestXPUMCTARGETDESC_H
