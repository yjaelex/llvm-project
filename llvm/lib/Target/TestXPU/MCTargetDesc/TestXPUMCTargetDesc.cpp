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

#include "TestXPUArch.h"
#include "TestXPUTargetStreamer.h"
#include "MCTargetDesc/TestXPUInstPrinter.h"
#include "MCTargetDesc/TestXPUMCAsmInfo.h"
#include "MCTargetDesc/TestXPUMCELFStreamer.h"
#include "MCTargetDesc/TestXPUMCInstrInfo.h"
#include "MCTargetDesc/TestXPUMCTargetDesc.h"
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

cl::opt<bool> llvm::TestXPUDisableCompound
  ("mno-compound",
   cl::desc("Disable looking for compound instructions for TestXPU"));

cl::opt<bool> llvm::TestXPUDisableDuplex
  ("mno-pairing",
   cl::desc("Disable looking for duplex instructions for TestXPU"));

namespace { // These flags are to be deprecated
cl::opt<bool> MV5("mv5", cl::Hidden, cl::desc("Build for TestXPU V5"),
                  cl::init(false));
cl::opt<bool> MV55("mv55", cl::Hidden, cl::desc("Build for TestXPU V55"),
                   cl::init(false));
cl::opt<bool> MV60("mv60", cl::Hidden, cl::desc("Build for TestXPU V60"),
                   cl::init(false));
cl::opt<bool> MV62("mv62", cl::Hidden, cl::desc("Build for TestXPU V62"),
                   cl::init(false));
cl::opt<bool> MV65("mv65", cl::Hidden, cl::desc("Build for TestXPU V65"),
                   cl::init(false));
cl::opt<bool> MV66("mv66", cl::Hidden, cl::desc("Build for TestXPU V66"),
                   cl::init(false));
cl::opt<bool> MV67("mv67", cl::Hidden, cl::desc("Build for TestXPU V67"),
                   cl::init(false));
cl::opt<bool> MV67T("mv67t", cl::Hidden, cl::desc("Build for TestXPU V67T"),
                    cl::init(false));
cl::opt<bool> MV68("mv68", cl::Hidden, cl::desc("Build for TestXPU V68"),
                   cl::init(false));

cl::opt<TestXPU::ArchEnum>
    EnableHVX("mhvx",
      cl::desc("Enable TestXPU Vector eXtensions"),
      cl::values(
        clEnumValN(TestXPU::ArchEnum::V60, "v60", "Build for HVX v60"),
        clEnumValN(TestXPU::ArchEnum::V62, "v62", "Build for HVX v62"),
        clEnumValN(TestXPU::ArchEnum::V65, "v65", "Build for HVX v65"),
        clEnumValN(TestXPU::ArchEnum::V66, "v66", "Build for HVX v66"),
        clEnumValN(TestXPU::ArchEnum::V67, "v67", "Build for HVX v67"),
        clEnumValN(TestXPU::ArchEnum::V68, "v68", "Build for HVX v68"),
        // Sentinel for no value specified.
        clEnumValN(TestXPU::ArchEnum::Generic, "", "")),
      // Sentinel for flag not present.
      cl::init(TestXPU::ArchEnum::NoArch), cl::ValueOptional);
} // namespace

static cl::opt<bool>
  DisableHVX("mno-hvx", cl::Hidden,
             cl::desc("Disable TestXPU Vector eXtensions"));


static StringRef DefaultArch = "TestXPUv60";

static StringRef TestXPUGetArchVariant() {
  if (MV5)
    return "TestXPUv5";
  if (MV55)
    return "TestXPUv55";
  if (MV60)
    return "TestXPUv60";
  if (MV62)
    return "TestXPUv62";
  if (MV65)
    return "TestXPUv65";
  if (MV66)
    return "TestXPUv66";
  if (MV67)
    return "TestXPUv67";
  if (MV67T)
    return "TestXPUv67t";
  if (MV68)
    return "TestXPUv68";
  return "";
}

StringRef TestXPU_MC::selectTestXPUCPU(StringRef CPU) {
  StringRef ArchV = TestXPUGetArchVariant();
  if (!ArchV.empty() && !CPU.empty()) {
    // Tiny cores have a "t" suffix that is discarded when creating a secondary
    // non-tiny subtarget.  See: addArchSubtarget
    std::pair<StringRef,StringRef> ArchP = ArchV.split('t');
    std::pair<StringRef,StringRef> CPUP = CPU.split('t');
    if (!ArchP.first.equals(CPUP.first))
        report_fatal_error("conflicting architectures specified.");
    return CPU;
  }
  if (ArchV.empty()) {
    if (CPU.empty())
      CPU = DefaultArch;
    return CPU;
  }
  return ArchV;
}

unsigned llvm::TestXPUGetLastSlot() { return TestXPUItinerariesV5FU::SLOT3; }

unsigned llvm::TestXPUConvertUnits(unsigned ItinUnits, unsigned *Lanes) {
  enum {
    CVI_NONE = 0,
    CVI_XLANE = 1 << 0,
    CVI_SHIFT = 1 << 1,
    CVI_MPY0 = 1 << 2,
    CVI_MPY1 = 1 << 3,
    CVI_ZW = 1 << 4
  };

  if (ItinUnits == TestXPUItinerariesV62FU::CVI_ALL ||
      ItinUnits == TestXPUItinerariesV62FU::CVI_ALL_NOMEM)
    return (*Lanes = 4, CVI_XLANE);
  else if (ItinUnits & TestXPUItinerariesV62FU::CVI_MPY01 &&
           ItinUnits & TestXPUItinerariesV62FU::CVI_XLSHF)
    return (*Lanes = 2, CVI_XLANE | CVI_MPY0);
  else if (ItinUnits & TestXPUItinerariesV62FU::CVI_MPY01)
    return (*Lanes = 2, CVI_MPY0);
  else if (ItinUnits & TestXPUItinerariesV62FU::CVI_XLSHF)
    return (*Lanes = 2, CVI_XLANE);
  else if (ItinUnits & TestXPUItinerariesV62FU::CVI_XLANE &&
           ItinUnits & TestXPUItinerariesV62FU::CVI_SHIFT &&
           ItinUnits & TestXPUItinerariesV62FU::CVI_MPY0 &&
           ItinUnits & TestXPUItinerariesV62FU::CVI_MPY1)
    return (*Lanes = 1, CVI_XLANE | CVI_SHIFT | CVI_MPY0 | CVI_MPY1);
  else if (ItinUnits & TestXPUItinerariesV62FU::CVI_XLANE &&
           ItinUnits & TestXPUItinerariesV62FU::CVI_SHIFT)
    return (*Lanes = 1, CVI_XLANE | CVI_SHIFT);
  else if (ItinUnits & TestXPUItinerariesV62FU::CVI_MPY0 &&
           ItinUnits & TestXPUItinerariesV62FU::CVI_MPY1)
    return (*Lanes = 1, CVI_MPY0 | CVI_MPY1);
  else if (ItinUnits == TestXPUItinerariesV62FU::CVI_ZW)
    return (*Lanes = 1, CVI_ZW);
  else if (ItinUnits == TestXPUItinerariesV62FU::CVI_XLANE)
    return (*Lanes = 1, CVI_XLANE);
  else if (ItinUnits == TestXPUItinerariesV62FU::CVI_SHIFT)
    return (*Lanes = 1, CVI_SHIFT);

  return (*Lanes = 0, CVI_NONE);
}


namespace llvm {
namespace TestXPUFUnits {
bool isSlot0Only(unsigned units) {
  return TestXPUItinerariesV62FU::SLOT0 == units;
}
} // namespace TestXPUFUnits
} // namespace llvm

namespace {

class TestXPUTargetAsmStreamer : public TestXPUTargetStreamer {
public:
  TestXPUTargetAsmStreamer(MCStreamer &S,
                           formatted_raw_ostream &OS,
                           bool isVerboseAsm,
                           MCInstPrinter &IP)
      : TestXPUTargetStreamer(S) {}

  void prettyPrintAsm(MCInstPrinter &InstPrinter, uint64_t Address,
                      const MCInst &Inst, const MCSubtargetInfo &STI,
                      raw_ostream &OS) override {
    assert(TestXPUMCInstrInfo::isBundle(Inst));
    assert(TestXPUMCInstrInfo::bundleSize(Inst) <= TestXPU_PACKET_SIZE);
    std::string Buffer;
    {
      raw_string_ostream TempStream(Buffer);
      InstPrinter.printInst(&Inst, Address, "", STI, TempStream);
    }
    StringRef Contents(Buffer);
    auto PacketBundle = Contents.rsplit('\n');
    auto HeadTail = PacketBundle.first.split('\n');
    StringRef Separator = "\n";
    StringRef Indent = "\t";
    OS << "\t{\n";
    while (!HeadTail.first.empty()) {
      StringRef InstTxt;
      auto Duplex = HeadTail.first.split('\v');
      if (!Duplex.second.empty()) {
        OS << Indent << Duplex.first << Separator;
        InstTxt = Duplex.second;
      } else if (!HeadTail.first.trim().startswith("immext")) {
        InstTxt = Duplex.first;
      }
      if (!InstTxt.empty())
        OS << Indent << InstTxt << Separator;
      HeadTail = HeadTail.second.split('\n');
    }

    if (TestXPUMCInstrInfo::isMemReorderDisabled(Inst))
      OS << "\n\t} :mem_noshuf" << PacketBundle.second;
    else
      OS << "\t}" << PacketBundle.second;
  }
};

class TestXPUTargetELFStreamer : public TestXPUTargetStreamer {
public:
  MCELFStreamer &getStreamer() {
    return static_cast<MCELFStreamer &>(Streamer);
  }
  TestXPUTargetELFStreamer(MCStreamer &S, MCSubtargetInfo const &STI)
      : TestXPUTargetStreamer(S) {
    MCAssembler &MCA = getStreamer().getAssembler();
    MCA.setELFHeaderEFlags(TestXPU_MC::GetELFFlags(STI));
  }


  void emitCommonSymbolSorted(MCSymbol *Symbol, uint64_t Size,
                              unsigned ByteAlignment,
                              unsigned AccessSize) override {
    TestXPUMCELFStreamer &TestXPUELFStreamer =
        static_cast<TestXPUMCELFStreamer &>(getStreamer());
    TestXPUELFStreamer.TestXPUMCEmitCommonSymbol(Symbol, Size, ByteAlignment,
                                                 AccessSize);
  }

  void emitLocalCommonSymbolSorted(MCSymbol *Symbol, uint64_t Size,
                                   unsigned ByteAlignment,
                                   unsigned AccessSize) override {
    TestXPUMCELFStreamer &TestXPUELFStreamer =
        static_cast<TestXPUMCELFStreamer &>(getStreamer());
    TestXPUELFStreamer.TestXPUMCEmitLocalCommonSymbol(
        Symbol, Size, ByteAlignment, AccessSize);
  }
};

} // end anonymous namespace

llvm::MCInstrInfo *llvm::createTestXPUMCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitTestXPUMCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createTestXPUMCRegisterInfo(const Triple &TT) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitTestXPUMCRegisterInfo(X, TestXPU::R31);
  return X;
}

static MCAsmInfo *createTestXPUMCAsmInfo(const MCRegisterInfo &MRI,
                                         const Triple &TT,
                                         const MCTargetOptions &Options) {
  MCAsmInfo *MAI = new TestXPUMCAsmInfo(TT);

  // VirtualFP = (R30 + #0).
  MCCFIInstruction Inst = MCCFIInstruction::cfiDefCfa(
      nullptr, MRI.getDwarfRegNum(TestXPU::R30, true), 0);
  MAI->addInitialFrameState(Inst);

  return MAI;
}

static MCInstPrinter *createTestXPUMCInstPrinter(const Triple &T,
                                                 unsigned SyntaxVariant,
                                                 const MCAsmInfo &MAI,
                                                 const MCInstrInfo &MII,
                                                 const MCRegisterInfo &MRI)
{
  if (SyntaxVariant == 0)
    return new TestXPUInstPrinter(MAI, MII, MRI);
  else
    return nullptr;
}

static MCTargetStreamer *
createMCAsmTargetStreamer(MCStreamer &S, formatted_raw_ostream &OS,
                          MCInstPrinter *IP, bool IsVerboseAsm) {
  return new TestXPUTargetAsmStreamer(S, OS, IsVerboseAsm, *IP);
}

static MCStreamer *createMCStreamer(Triple const &T, MCContext &Context,
                                    std::unique_ptr<MCAsmBackend> &&MAB,
                                    std::unique_ptr<MCObjectWriter> &&OW,
                                    std::unique_ptr<MCCodeEmitter> &&Emitter,
                                    bool RelaxAll) {
  return createTestXPUELFStreamer(T, Context, std::move(MAB), std::move(OW),
                                  std::move(Emitter));
}

static MCTargetStreamer *
createTestXPUObjectTargetStreamer(MCStreamer &S, const MCSubtargetInfo &STI) {
  return new TestXPUTargetELFStreamer(S, STI);
}

static void LLVM_ATTRIBUTE_UNUSED clearFeature(MCSubtargetInfo* STI, uint64_t F) {
  if (STI->getFeatureBits()[F])
    STI->ToggleFeature(F);
}

static bool LLVM_ATTRIBUTE_UNUSED checkFeature(MCSubtargetInfo* STI, uint64_t F) {
  return STI->getFeatureBits()[F];
}

namespace {
std::string selectTestXPUFS(StringRef CPU, StringRef FS) {
  SmallVector<StringRef, 3> Result;
  if (!FS.empty())
    Result.push_back(FS);

  switch (EnableHVX) {
  case TestXPU::ArchEnum::V5:
  case TestXPU::ArchEnum::V55:
    break;
  case TestXPU::ArchEnum::V60:
    Result.push_back("+hvxv60");
    break;
  case TestXPU::ArchEnum::V62:
    Result.push_back("+hvxv62");
    break;
  case TestXPU::ArchEnum::V65:
    Result.push_back("+hvxv65");
    break;
  case TestXPU::ArchEnum::V66:
    Result.push_back("+hvxv66");
    break;
  case TestXPU::ArchEnum::V67:
    Result.push_back("+hvxv67");
    break;
  case TestXPU::ArchEnum::V68:
    Result.push_back("+hvxv68");
    break;
  case TestXPU::ArchEnum::Generic:{
    Result.push_back(StringSwitch<StringRef>(CPU)
             .Case("TestXPUv60", "+hvxv60")
             .Case("TestXPUv62", "+hvxv62")
             .Case("TestXPUv65", "+hvxv65")
             .Case("TestXPUv66", "+hvxv66")
             .Case("TestXPUv67", "+hvxv67")
             .Case("TestXPUv67t", "+hvxv67")
             .Case("TestXPUv68", "+hvxv68"));
    break;
  }
  case TestXPU::ArchEnum::NoArch:
    // Sentinel if -mhvx isn't specified
    break;
  }
  return join(Result.begin(), Result.end(), ",");
}
}

static bool isCPUValid(const std::string &CPU) {
  return TestXPU::CpuTable.find(CPU) != TestXPU::CpuTable.cend();
}

namespace {
std::pair<std::string, std::string> selectCPUAndFS(StringRef CPU,
                                                   StringRef FS) {
  std::pair<std::string, std::string> Result;
  Result.first = std::string(TestXPU_MC::selectTestXPUCPU(CPU));
  Result.second = selectTestXPUFS(Result.first, FS);
  return Result;
}
std::mutex ArchSubtargetMutex;
std::unordered_map<std::string, std::unique_ptr<MCSubtargetInfo const>>
    ArchSubtarget;
} // namespace

MCSubtargetInfo const *
TestXPU_MC::getArchSubtarget(MCSubtargetInfo const *STI) {
  std::lock_guard<std::mutex> Lock(ArchSubtargetMutex);
  auto Existing = ArchSubtarget.find(std::string(STI->getCPU()));
  if (Existing == ArchSubtarget.end())
    return nullptr;
  return Existing->second.get();
}

FeatureBitset TestXPU_MC::completeHVXFeatures(const FeatureBitset &S) {
  using namespace TestXPU;
  // Make sure that +hvx-length turns hvx on, and that "hvx" alone
  // turns on hvxvNN, corresponding to the existing ArchVNN.
  FeatureBitset FB = S;
  unsigned CpuArch = ArchV5;
  for (unsigned F : {ArchV68, ArchV67, ArchV66, ArchV65, ArchV62, ArchV60,
                     ArchV55, ArchV5}) {
    if (!FB.test(F))
      continue;
    CpuArch = F;
    break;
  }
  bool UseHvx = false;
  for (unsigned F : {ExtensionHVX, ExtensionHVX64B, ExtensionHVX128B}) {
    if (!FB.test(F))
      continue;
    UseHvx = true;
    break;
  }
  bool HasHvxVer = false;
  for (unsigned F : {ExtensionHVXV60, ExtensionHVXV62, ExtensionHVXV65,
                     ExtensionHVXV66, ExtensionHVXV67, ExtensionHVXV68}) {
    if (!FB.test(F))
      continue;
    HasHvxVer = true;
    UseHvx = true;
    break;
  }

  if (!UseHvx || HasHvxVer)
    return FB;

  // HasHvxVer is false, and UseHvx is true.
  switch (CpuArch) {
    case ArchV68:
      FB.set(ExtensionHVXV68);
      LLVM_FALLTHROUGH;
    case ArchV67:
      FB.set(ExtensionHVXV67);
      LLVM_FALLTHROUGH;
    case ArchV66:
      FB.set(ExtensionHVXV66);
      LLVM_FALLTHROUGH;
    case ArchV65:
      FB.set(ExtensionHVXV65);
      LLVM_FALLTHROUGH;
    case ArchV62:
      FB.set(ExtensionHVXV62);
      LLVM_FALLTHROUGH;
    case ArchV60:
      FB.set(ExtensionHVXV60);
      break;
  }
  return FB;
}

MCSubtargetInfo *TestXPU_MC::createTestXPUMCSubtargetInfo(const Triple &TT,
                                                          StringRef CPU,
                                                          StringRef FS) {
  std::pair<std::string, std::string> Features = selectCPUAndFS(CPU, FS);
  StringRef CPUName = Features.first;
  StringRef ArchFS = Features.second;

  MCSubtargetInfo *X = createTestXPUMCSubtargetInfoImpl(
      TT, CPUName, /*TuneCPU*/ CPUName, ArchFS);
  if (X != nullptr && (CPUName == "TestXPUv67t"))
    addArchSubtarget(X, ArchFS);

  if (CPU.equals("help"))
      exit(0);

  if (!isCPUValid(CPUName.str())) {
    errs() << "error: invalid CPU \"" << CPUName.str().c_str()
           << "\" specified\n";
    return nullptr;
  }

  if (TestXPUDisableDuplex) {
    llvm::FeatureBitset Features = X->getFeatureBits();
    X->setFeatureBits(Features.reset(TestXPU::FeatureDuplex));
  }

  X->setFeatureBits(completeHVXFeatures(X->getFeatureBits()));

  // The Z-buffer instructions are grandfathered in for current
  // architectures but omitted for new ones.  Future instruction
  // sets may introduce new/conflicting z-buffer instructions.
  const bool ZRegOnDefault =
      (CPUName == "TestXPUv67") || (CPUName == "TestXPUv66");
  if (ZRegOnDefault) {
    llvm::FeatureBitset Features = X->getFeatureBits();
    X->setFeatureBits(Features.set(TestXPU::ExtensionZReg));
  }

  return X;
}

void TestXPU_MC::addArchSubtarget(MCSubtargetInfo const *STI,
                                  StringRef FS) {
  assert(STI != nullptr);
  if (STI->getCPU().contains("t")) {
    auto ArchSTI = createTestXPUMCSubtargetInfo(
        STI->getTargetTriple(),
        STI->getCPU().substr(0, STI->getCPU().size() - 1), FS);
    std::lock_guard<std::mutex> Lock(ArchSubtargetMutex);
    ArchSubtarget[std::string(STI->getCPU())] =
        std::unique_ptr<MCSubtargetInfo const>(ArchSTI);
  }
}

unsigned TestXPU_MC::GetELFFlags(const MCSubtargetInfo &STI) {
  static std::map<StringRef,unsigned> ElfFlags = {
    {"TestXPUv5",  ELF::EF_TestXPU_MACH_V5},
    {"TestXPUv55", ELF::EF_TestXPU_MACH_V55},
    {"TestXPUv60", ELF::EF_TestXPU_MACH_V60},
    {"TestXPUv62", ELF::EF_TestXPU_MACH_V62},
    {"TestXPUv65", ELF::EF_TestXPU_MACH_V65},
    {"TestXPUv66", ELF::EF_TestXPU_MACH_V66},
    {"TestXPUv67", ELF::EF_TestXPU_MACH_V67},
    {"TestXPUv67t", ELF::EF_TestXPU_MACH_V67T},
    {"TestXPUv68", ELF::EF_TestXPU_MACH_V68},
  };

  auto F = ElfFlags.find(STI.getCPU());
  assert(F != ElfFlags.end() && "Unrecognized Architecture");
  return F->second;
}

llvm::ArrayRef<MCPhysReg> TestXPU_MC::GetVectRegRev() {
  return makeArrayRef(VectRegRev);
}

namespace {
class TestXPUMCInstrAnalysis : public MCInstrAnalysis {
public:
  TestXPUMCInstrAnalysis(MCInstrInfo const *Info) : MCInstrAnalysis(Info) {}

  bool isUnconditionalBranch(MCInst const &Inst) const override {
    //assert(!TestXPUMCInstrInfo::isBundle(Inst));
    return MCInstrAnalysis::isUnconditionalBranch(Inst);
  }

  bool isConditionalBranch(MCInst const &Inst) const override {
    //assert(!TestXPUMCInstrInfo::isBundle(Inst));
    return MCInstrAnalysis::isConditionalBranch(Inst);
  }

  bool evaluateBranch(MCInst const &Inst, uint64_t Addr,
                      uint64_t Size, uint64_t &Target) const override {
    if (!(isCall(Inst) || isUnconditionalBranch(Inst) ||
          isConditionalBranch(Inst)))
      return false;

    //assert(!TestXPUMCInstrInfo::isBundle(Inst));
    if(!TestXPUMCInstrInfo::isExtendable(*Info, Inst))
      return false;
    auto const &Extended(TestXPUMCInstrInfo::getExtendableOperand(*Info, Inst));
    assert(Extended.isExpr());
    int64_t Value;
    if(!Extended.getExpr()->evaluateAsAbsolute(Value))
      return false;
    Target = Value;
    return true;
  }
};
}

static MCInstrAnalysis *createTestXPUMCInstrAnalysis(const MCInstrInfo *Info) {
  return new TestXPUMCInstrAnalysis(Info);
}

// Force static initialization.
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeTestXPUTargetMC() {
  // Register the MC asm info.
  RegisterMCAsmInfoFn X(getTheTestXPUTarget(), createTestXPUMCAsmInfo);

  // Register the MC instruction info.
  TargetRegistry::RegisterMCInstrInfo(getTheTestXPUTarget(),
                                      createTestXPUMCInstrInfo);

  // Register the MC register info.
  TargetRegistry::RegisterMCRegInfo(getTheTestXPUTarget(),
                                    createTestXPUMCRegisterInfo);

  // Register the MC subtarget info.
  TargetRegistry::RegisterMCSubtargetInfo(getTheTestXPUTarget(),
    TestXPU_MC::createTestXPUMCSubtargetInfo);

  // Register the MC Code Emitter
  TargetRegistry::RegisterMCCodeEmitter(getTheTestXPUTarget(),
                                        createTestXPUMCCodeEmitter);

  // Register the asm backend
  TargetRegistry::RegisterMCAsmBackend(getTheTestXPUTarget(),
                                       createTestXPUAsmBackend);


  // Register the MC instruction analyzer.
  TargetRegistry::RegisterMCInstrAnalysis(getTheTestXPUTarget(),
                                          createTestXPUMCInstrAnalysis);

  // Register the obj streamer
  TargetRegistry::RegisterELFStreamer(getTheTestXPUTarget(),
                                      createMCStreamer);

  // Register the obj target streamer
  TargetRegistry::RegisterObjectTargetStreamer(getTheTestXPUTarget(),
                                      createTestXPUObjectTargetStreamer);

  // Register the asm streamer
  TargetRegistry::RegisterAsmTargetStreamer(getTheTestXPUTarget(),
                                            createMCAsmTargetStreamer);

  // Register the MC Inst Printer
  TargetRegistry::RegisterMCInstPrinter(getTheTestXPUTarget(),
                                        createTestXPUMCInstPrinter);
}
