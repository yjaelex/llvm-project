//===-- TestXPUMCTargetDesc.cpp - TestXPU Target Descriptions -------------===//
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

#include "MCTargetDesc/TestXPUMCTargetDesc.h"
#include "MCTargetDesc/TestXPUMCAsmInfo.h"
#include "TargetInfo/TestXPUTargetInfo.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDwarf.h"
#include "llvm/MC/MCELFStreamer.h"
#include "llvm/MC/MCInstrAnalysis.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/raw_ostream.h"
#include <cassert>
#include <cstdint>
#include <mutex>
#include <new>
#include <string>
#include <unordered_map>

using namespace llvm;

#define GET_INSTRINFO_MC_DESC
#include "TestXPUGenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "TestXPUGenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "TestXPUGenRegisterInfo.inc"

static MCInstPrinter *createTestXPUMCInstPrinter(const Triple &T,
                                               unsigned SyntaxVariant,
                                               const MCAsmInfo &MAI,
                                               const MCInstrInfo &MII,
                                               const MCRegisterInfo &MRI) {
  return nullptr;
}

static MCAsmInfo *createTestXPUMCAsmInfo(const MCRegisterInfo &MRI,
                                       const Triple &TT,
                                       const MCTargetOptions &Options) {
  MCAsmInfo * MAI = new TestXPUMCAsmInfo(TT);
  unsigned SP = MRI.getDwarfRegNum(TestXPU::SP, true);
  MCCFIInstruction Inst = MCCFIInstruction::cfiDefCfa(nullptr, SP, 0);
  MAI->addInitialFrameState(Inst);
  return MAI;
}

static MCInstrInfo *createTestXPUMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitTestXPUMCInstrInfo(X);
  return X;
}

// TestXPU::SP  --> stack pointer
// TestXPU::SR  --> status register
// TestXPU::R13 --> RA reg: return address reg
static MCRegisterInfo *createTestXPUMCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitTestXPUMCRegisterInfo(X, TestXPU::R13);
  return X;
}

static MCSubtargetInfo *createTestXPUMCSubtargetInfo(const Triple &TT, StringRef CPU, StringRef FS) {
  if (CPU.empty())
    CPU = "testxpu";
  return createTestXPUMCSubtargetInfoImpl(TT, CPU, /*TuneCPU*/ CPU, FS);
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeTestXPUTargetMC() {
  // Register the MC asm info.
  RegisterMCAsmInfoFn X(getTheTestXPUTarget(), createTestXPUMCAsmInfo);

  TargetRegistry::RegisterMCInstrInfo(getTheTestXPUTarget(), createTestXPUMCInstrInfo);

  // Register the MC register info.
  TargetRegistry::RegisterMCRegInfo(getTheTestXPUTarget(), createTestXPUMCRegisterInfo);

  // Register the MC subtarget info.
  TargetRegistry::RegisterMCSubtargetInfo(getTheTestXPUTarget(), createTestXPUMCSubtargetInfo);

  // Register the MC Code Emitter.
  //TargetRegistry::RegisterMCCodeEmitter(getTheTestXPUTarget(), createTestXPUMCCodeEmitter);

  // Register the asm backend.
  // TargetRegistry::RegisterMCAsmBackend(getTheTestXPUTarget(), createTestXPUAsmBackend);
}
