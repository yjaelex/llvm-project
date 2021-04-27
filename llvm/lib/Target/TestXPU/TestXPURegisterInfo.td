//===-- TestXPURegisterInfo.td - TestXPU Register defs -----------*- tablegen -*-===//
//
//                    The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.

//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Declarations that describe the TestXPU register file
//===----------------------------------------------------------------------===//

//include "llvm/Target/Target.td"

// We have banks of 16 registers each.
class TestXPUReg<bits<16> Enc, string n> : Register<n> {
  // For tablegen(... -gen-emitter) in CMakeLists.txt
  let HWEncoding = Enc;
  let Namespace = "TestXPU";
}

//===----------------------------------------------------------------------===//
//@Registers
//===----------------------------------------------------------------------===//
// The register string, such as "9" or "gp" will show on "llvm-objdump -d"
//@ All registers definition
let Namespace = "TestXPU" in {
  //@ General Purpose Registers
  def R0   : TestXPUReg<0,   "0">,    DwarfRegNum<[0]>;
  def R1   : TestXPUReg<1,   "1">,    DwarfRegNum<[1]>;
  def R2   : TestXPUReg<2,   "2">,    DwarfRegNum<[2]>;
  def R3   : TestXPUReg<3,   "3">,    DwarfRegNum<[3]>;
  def R4   : TestXPUReg<4,   "4">,    DwarfRegNum<[4]>;
  def R5   : TestXPUReg<5,   "5">,    DwarfRegNum<[5]>;
  def R6   : TestXPUReg<6,   "6">,    DwarfRegNum<[6]>;
  def R7   : TestXPUReg<7,   "7">,    DwarfRegNum<[7]>;
  def R8   : TestXPUReg<8,   "8">,    DwarfRegNum<[8]>;
  def R9   : TestXPUReg<9,   "9">,    DwarfRegNum<[9]>;
  def R10   : TestXPUReg<10,  "10">,   DwarfRegNum<[10]>;
  def R11   : TestXPUReg<11,  "11">,   DwarfRegNum<[11]>;
  def R12   : TestXPUReg<12,  "12">,   DwarfRegNum<[12]>;
  def R13   : TestXPUReg<13,  "13">,   DwarfRegNum<[13]>;

  // stack pointer
  def SP   : TestXPUReg<14,  "sp">,   DwarfRegNum<[14]>;
  // status register
  def SR   : TestXPUReg<15,  "sr">,   DwarfRegNum<[15]>;
}

//===----------------------------------------------------------------------===//
// @Register Classes
//===----------------------------------------------------------------------===//

def CPURegs : RegisterClass<"TestXPU", [i32], 32, (add
  // Reserved
  R0,
  R1, R2, R3, R4, R5, R6, R7, R8, R9,
  R10, R11, R12, R13, SP, SR)>;

