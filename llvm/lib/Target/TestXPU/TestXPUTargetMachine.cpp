//===-- TestXPUTargetMachine.cpp - Define TargetMachine for TestXPU -------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Implements the info about TestXPU target spec.
//
//===----------------------------------------------------------------------===//

#include "TestXPUTargetMachine.h"
//#include "TestXPU.h"
//#include "TargetInfo/TestXPUTargetInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Scalar.h"

using namespace llvm;

static cl::opt<bool> EnableCExtOpt("TestXPU-cext", cl::Hidden, cl::ZeroOrMore,
  cl::init(true), cl::desc("Enable TestXPU constant-extender optimization"));

namespace llvm {
  //void initializeTestXPUBitSimplifyPass(PassRegistry&);

Target & getTheTestXPUTarget() {
  static Target TheTestXPUTarget;
  return TheTestXPUTarget;
}

} // end namespace llvm;

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeTestXPUTarget() {
  // Register the target.
  RegisterTargetMachine<TestXPUTargetMachine> X(getTheTestXPUTarget());

}

#if 0

TestXPUTargetMachine::TestXPUTargetMachine(const Module &M, const std::string &FS)
  : DataLayout("E-p:32:32-f128:128:128"),
    Subtarget(M, FS), InstrInfo(Subtarget),
    FrameInfo(TargetFrameInfo::StackGrowsDown, 8, 0) {
}

const TestXPUSubtarget *
TestXPUTargetMachine::getSubtargetImpl(const Function &F) const {
  AttributeList FnAttrs = F.getAttributes();
  Attribute CPUAttr =
      FnAttrs.getAttribute(AttributeList::FunctionIndex, "target-cpu");
  Attribute FSAttr =
      FnAttrs.getAttribute(AttributeList::FunctionIndex, "target-features");

  std::string CPU =
      CPUAttr.isValid() ? CPUAttr.getValueAsString().str() : TargetCPU;
  std::string FS =
      FSAttr.isValid() ? FSAttr.getValueAsString().str() : TargetFS;
  // Append the preexisting target features last, so that +mattr overrides
  // the "unsafe-fp-math" function attribute.
  // Creating a separate target feature is not strictly necessary, it only
  // exists to make "unsafe-fp-math" force creating a new subtarget.

  if (FnAttrs.hasFnAttribute("unsafe-fp-math") &&
      F.getFnAttribute("unsafe-fp-math").getValueAsString() == "true")
    FS = FS.empty() ? "+unsafe-fp" : "+unsafe-fp," + FS;

  auto &I = SubtargetMap[CPU + FS];
  if (!I) {
    // This needs to be done before we create a new subtarget since any
    // creation will depend on the TM and the code generation flags on the
    // function that reside in TargetOptions.
    resetTargetOptions(F);
    I = std::make_unique<TestXPUSubtarget>(TargetTriple, CPU, FS, *this);
  }
  return I.get();
}


TestXPUTargetMachine::~TestXPUTargetMachine() {}

#endif
