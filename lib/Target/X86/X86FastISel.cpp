//===-- X86FastISel.cpp - X86 FastISel implementation ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the X86-specific support for the FastISel class. Much
// of the target-specific code is generated by tablegen in the file
// X86GenFastISel.inc, which is #included here.
//
//===----------------------------------------------------------------------===//

#include "X86.h"
#include "X86InstrBuilder.h"
#include "X86ISelLowering.h"
#include "X86RegisterInfo.h"
#include "X86Subtarget.h"
#include "X86TargetMachine.h"
#include "llvm/CodeGen/FastISel.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"

using namespace llvm;

class X86FastISel : public FastISel {
  /// Subtarget - Keep a pointer to the X86Subtarget around so that we can
  /// make the right decision when generating code for different targets.
  const X86Subtarget *Subtarget;
    
public:
  explicit X86FastISel(MachineFunction &mf) : FastISel(mf) {
    Subtarget = &TM.getSubtarget<X86Subtarget>();
  }

  virtual bool
    TargetSelectInstruction(Instruction *I,
                            DenseMap<const Value *, unsigned> &ValueMap,
                      DenseMap<const BasicBlock *, MachineBasicBlock *> &MBBMap,
                            MachineBasicBlock *MBB);

#include "X86GenFastISel.inc"

private:
  bool X86SelectConstAddr(Value *V,
                          DenseMap<const Value *, unsigned> &ValueMap,
                          MachineBasicBlock *MBB, unsigned &Op0);

  bool X86SelectLoad(Instruction *I,
                     DenseMap<const Value *, unsigned> &ValueMap,
                     MachineBasicBlock *MBB);
};

/// X86SelectConstAddr - Select and emit code to materialize constant address.
/// 
bool X86FastISel::X86SelectConstAddr(Value *V,
                                    DenseMap<const Value *, unsigned> &ValueMap,
                                    MachineBasicBlock *MBB,
                                    unsigned &Op0) {
  // FIXME: Only GlobalAddress for now.
  GlobalValue *GV = dyn_cast<GlobalValue>(V);
  if (!GV)
    return false;

  if (Subtarget->GVRequiresExtraLoad(GV, TM, false)) {
    // Issue load from stub if necessary.
    unsigned Opc = 0;
    const TargetRegisterClass *RC = NULL;
    if (TLI.getPointerTy() == MVT::i32) {
      Opc = X86::MOV32rm;
      RC  = X86::GR32RegisterClass;
    } else {
      Opc = X86::MOV64rm;
      RC  = X86::GR64RegisterClass;
    }
    Op0 = createResultReg(RC);
    X86AddressMode AM;
    AM.GV = GV;
    addFullAddress(BuildMI(MBB, TII.get(Opc), Op0), AM);
  }
  return true;
}

/// X86SelectLoad - Select and emit code to implement load instructions.
///
bool X86FastISel::X86SelectLoad(Instruction *I,
                                DenseMap<const Value *, unsigned> &ValueMap,
                                MachineBasicBlock *MBB)  {
  MVT VT = MVT::getMVT(I->getType(), /*HandleUnknown=*/true);
  if (VT == MVT::Other || !VT.isSimple())
    // Unhandled type. Halt "fast" selection and bail.
    return false;
  if (VT == MVT::iPTR)
    // Use pointer type.
    VT = TLI.getPointerTy();
  // We only handle legal types. For example, on x86-32 the instruction
  // selector contains all of the 64-bit instructions from x86-64,
  // under the assumption that i64 won't be used if the target doesn't
  // support it.
  if (!TLI.isTypeLegal(VT))
    return false;

  Value *V = I->getOperand(0);
  unsigned Op0 = getRegForValue(V, ValueMap);
  if (Op0 == 0) {
    // Handle constant load address.
    if (!isa<Constant>(V) || !X86SelectConstAddr(V, ValueMap, MBB, Op0))
      // Unhandled operand. Halt "fast" selection and bail.
      return false;    
  }

  // Get opcode and regclass of the output for the given load instruction.
  unsigned Opc = 0;
  const TargetRegisterClass *RC = NULL;
  switch (VT.getSimpleVT()) {
  default: return false;
  case MVT::i8:
    Opc = X86::MOV8rm;
    RC  = X86::GR8RegisterClass;
    break;
  case MVT::i16:
    Opc = X86::MOV16rm;
    RC  = X86::GR16RegisterClass;
    break;
  case MVT::i32:
    Opc = X86::MOV32rm;
    RC  = X86::GR32RegisterClass;
    break;
  case MVT::i64:
    // Must be in x86-64 mode.
    Opc = X86::MOV64rm;
    RC  = X86::GR64RegisterClass;
    break;
  case MVT::f32:
    if (Subtarget->hasSSE1()) {
      Opc = X86::MOVSSrm;
      RC  = X86::FR32RegisterClass;
    } else {
      Opc = X86::LD_Fp32m;
      RC  = X86::RFP32RegisterClass;
    }
    break;
  case MVT::f64:
    if (Subtarget->hasSSE2()) {
      Opc = X86::MOVSDrm;
      RC  = X86::FR64RegisterClass;
    } else {
      Opc = X86::LD_Fp64m;
      RC  = X86::RFP64RegisterClass;
    }
    break;
  case MVT::f80:
    Opc = X86::LD_Fp80m;
    RC  = X86::RFP80RegisterClass;
    break;
  }

  unsigned ResultReg = createResultReg(RC);
  X86AddressMode AM;
  if (Op0)
    // Address is in register.
    AM.Base.Reg = Op0;
  else
    AM.GV = cast<GlobalValue>(V);
  addFullAddress(BuildMI(MBB, TII.get(Opc), ResultReg), AM);
  UpdateValueMap(I, ResultReg, ValueMap);
  return true;
}


bool
X86FastISel::TargetSelectInstruction(Instruction *I,
                                  DenseMap<const Value *, unsigned> &ValueMap,
                      DenseMap<const BasicBlock *, MachineBasicBlock *> &MBBMap,
                                  MachineBasicBlock *MBB)  {
  switch (I->getOpcode()) {
  default: break;
  case Instruction::Load:
    return X86SelectLoad(I, ValueMap, MBB);
  }

  return false;
}

namespace llvm {
  llvm::FastISel *X86::createFastISel(MachineFunction &mf) {
    return new X86FastISel(mf);
  }
}
