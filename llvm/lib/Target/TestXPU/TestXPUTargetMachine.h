#ifndef LLVM_LIB_TARGET_TestXPU_TestXPUTARGETMACHINE_H
#define LLVM_LIB_TARGET_TestXPU_TestXPUTARGETMACHINE_H

#include "llvm/Target/TargetMachine.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"


namespace llvm {

class Module;

#if 1

static std::string computeDataLayout(const Triple &T) {
  std::string Ret = "E-p:32:32-f128:128:128";

  return Ret;
}

static Reloc::Model getEffectiveRelocModel(Optional<Reloc::Model> RM) {
  return RM.getValueOr(Reloc::Static);
}

static CodeModel::Model
getEffectiveTestXPUCodeModel(Optional<CodeModel::Model> CM, Reloc::Model RM) {
  if (CM) {
    if (*CM == CodeModel::Tiny)
      report_fatal_error("Target does not support the tiny CodeModel", false);
    if (*CM == CodeModel::Kernel)
      report_fatal_error("Target does not support the kernel CodeModel", false);
    return *CM;
  }

  return CodeModel::Small;
}

class TestXPUTargetMachine : public LLVMTargetMachine {
public:
  TestXPUTargetMachine(const Target &T, const Triple &TT,
                                     StringRef CPU, StringRef FS,
                                     const TargetOptions &Options,
                                     Optional<Reloc::Model> RM,
                                     Optional<CodeModel::Model> CM,
                                     CodeGenOpt::Level OL, bool JIT)
    : LLVMTargetMachine(T, computeDataLayout(TT), TT,
                        CPU, FS, Options, getEffectiveRelocModel(RM),
                        getEffectiveCodeModel(CM, CodeModel::Small), OL)
{

}
};

#else

class TestXPUTargetMachine : public LLVMTargetMachine {

public:
  ~TestXPUTargetMachine() override;
 // const TestXPUSubtarget *getSubtargetImpl(const Function &F) const override;

  const DataLayout DataLayout;       // Calculates type size & alignment
  TestXPUSubtarget Subtarget;
  TestXPUInstrInfo InstrInfo;
  TargetFrameInfo FrameInfo;

protected:
  virtual const TargetAsmInfo *createTargetAsmInfo() const;

public:
  TestXPUTargetMachine(const Module &M, const std::string &FS);

  virtual const TestXPUInstrInfo *getInstrInfo() const {return &InstrInfo; }
  virtual const TargetFrameInfo *getFrameInfo() const {return &FrameInfo; }
  virtual const TargetSubtarget *getSubtargetImpl() const{return &Subtarget; }
  virtual const TargetRegisterInfo *getRegisterInfo() const {
    return &InstrInfo.getRegisterInfo();
  }
  virtual const DataLayout *getDataLayout() const { return &DataLayout; }
  static unsigned getModuleMatchQuality(const Module &M);

  // Pass Pipeline Configuration
  virtual bool addInstSelector(PassManagerBase &PM, bool Fast);
  virtual bool addPreEmitPass(PassManagerBase &PM, bool Fast);
};
#endif

} // end namespace llvm

#endif
