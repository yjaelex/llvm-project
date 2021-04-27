//===-- TestXPUTargetInfo.cpp - TestXPU Target Implementation ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "TargetInfo/TestXPUTargetInfo.h"
#include "llvm/Support/TargetRegistry.h"
using namespace llvm;

Target &llvm::getTheTestXPUTarget() {
  static Target TheTestXPUTarget;
  return TheTestXPUTarget;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeTestXPUTargetInfo() {
  RegisterTarget<Triple::TestXPU, /*HasJIT=*/true> X(
      getTheTestXPUTarget(), "TestXPU", "TestXPU", "TestXPU");
}
