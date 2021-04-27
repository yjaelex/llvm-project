//===-- TestXPUInstrFormats.td - TestXPU Instruction Formats -----*- tablegen -*-===//
//
//                    The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.


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

//
//===----------------------------------------------------------------------===//
//
// Describe TestXPU instructions format
//
// CPU INSTRUCTION FORMATS
//
// opcode - operation code.
// ra     - dst reg, only used on 3 regs instr.
// rb     - src reg.
// rc     - src reg (on a 3 reg instr).
// cx     - immediate
//
//===----------------------------------------------------------------------===//

// Format specifies the encoding used by the instruction. This is part of the
// ad-hoc solution used to emit machine instruction encodings by our machine
// code emitter.
class Format<bits<4> val>
{
  bits<4> Value = val;
}

def Pseudo      : Format<0>;
def FrmA        : Format<1>;
def FrmL        : Format<2>;
def FrmJ        : Format<3>;
def FrmOther    : Format<4>; // Instruction w/ a custom format

// Generic TestXPU Format
class TestXPUInst<dag outs, dag ins, string asmStr, list<dag> pattern,
               InstrItinClass itin, Format f> : Instruction
{
  // Inst and Size: for tablegen(... -gen-emitter) and
  // tablegen(... -gen-disassembler) in CMakeLists.txt
  field bits<32> Inst;
  Format Form = f;

  let Namespace = "TestXPU";

  let Size = 4;

  bits<8> Opcode = 0;

  // Top 8 bits are the 'opcode' field
  let Inst{31-24} = Opcode;

  let OutOperandList = outs;
  let InOperandList  = ins;

  let AsmString = asmStr;
  let Pattern   = pattern;
  let Itinerary = itin;

  // Attributes specific to TestXPU instructions
  bits<4> FormBits = Form.Value;

  let TSFlags{3-0} = FormBits;

  let DecoderNamespace = "TestXPU";

  field bits<32> SoftFail = 0;
}

//===----------------------------------------------------------------------===//
// Format A instruction class in TestXPU : <|opcode|ra|rb|rc|cx|>
//===----------------------------------------------------------------------===//

class FA<bits<8> op, dag outs, dag ins, string asmStr,
         list<dag> pattern, InstrItinClass itin>
  : TestXPUInst<outs, ins, asmStr, pattern, itin, FrmA>
{
  bits<4>  ra;
  bits<4>  rb;
  bits<4>  rc;
  bits<12> shamt;

  let Opcode = op;

  let Inst{23-20} = ra;
  let Inst{19-16} = rb;
  let Inst{15-12} = rc;
  let Inst{11-0}  = shamt;
}

//===----------------------------------------------------------------------===//
// Format L instruction class in TestXPU : <|opcode|ra|rb|cx|>
//===----------------------------------------------------------------------===//

class FL<bits<8> op, dag outs, dag ins, string asmStr,
         list<dag> pattern, InstrItinClass itin>
  : TestXPUInst<outs, ins, asmStr, pattern, itin, FrmL>
{
  bits<4>  ra;
  bits<4>  rb;
  bits<16> imm16;

  let Opcode = op;

  let Inst{23-20} = ra;
  let Inst{19-16} = rb;
  let Inst{15-0}  = imm16;
}

//===----------------------------------------------------------------------===//
// Format J instruction class in TestXPU : <|opcode|address|>
//===----------------------------------------------------------------------===//

class FJ<bits<8> op, dag outs, dag ins, string asmStr,
         list<dag> pattern, InstrItinClass itin>
  : TestXPUInst<outs, ins, asmStr, pattern, itin, FrmJ>
{
  bits<24> addr;

  let Opcode = op;

  let Inst{23-0} = addr;
}
