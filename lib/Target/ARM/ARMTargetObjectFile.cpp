//===-- llvm/Target/ARMTargetObjectFile.cpp - ARM Object Info Impl --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ARMTargetObjectFile.h"
#include "ARMSubtarget.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/Support/Dwarf.h"
#include "llvm/Support/ELF.h"
#include "llvm/Target/TargetMachine.h"
using namespace llvm;
using namespace dwarf;

//===----------------------------------------------------------------------===//
//                               ELF Target
//===----------------------------------------------------------------------===//

void ARMElfTargetObjectFile::Initialize(MCContext &Ctx,
                                        const TargetMachine &TM) {
  TargetLoweringObjectFileELF::Initialize(Ctx, TM);

  if (TM.getSubtarget<ARMSubtarget>().isAAPCS_ABI()) {
    StaticCtorSection =
      getContext().getELFSection(".init_array", ELF::SHT_INIT_ARRAY,
                                 ELF::SHF_WRITE |
                                 ELF::SHF_ALLOC,
                                 SectionKind::getDataRel());
    StaticDtorSection =
      getContext().getELFSection(".fini_array", ELF::SHT_FINI_ARRAY,
                                 ELF::SHF_WRITE |
                                 ELF::SHF_ALLOC,
                                 SectionKind::getDataRel());
  }
  
  AttributesSection =
    getContext().getELFSection(".ARM.attributes",
                               ELF::SHT_ARM_ATTRIBUTES,
                               0,
                               SectionKind::getMetadata());
}

unsigned ARMElfTargetObjectFile::getPersonalityEncoding() const {
    return DW_EH_PE_indirect | DW_EH_PE_pcrel | DW_EH_PE_sdata4;
}

unsigned ARMElfTargetObjectFile::getLSDAEncoding() const {
    return DW_EH_PE_pcrel | DW_EH_PE_sdata4;
}

unsigned ARMElfTargetObjectFile::getFDEEncoding() const {
    return DW_EH_PE_pcrel | DW_EH_PE_sdata4;
}

unsigned ARMElfTargetObjectFile::getTTypeEncoding() const {
  return DW_EH_PE_indirect | DW_EH_PE_pcrel | DW_EH_PE_sdata4;
}

//===----------------------------------------------------------------------===//
//                               MACHO Target
//===----------------------------------------------------------------------===//

void ARMMachOTargetObjectFile::Initialize(MCContext &Ctx,
                                        const TargetMachine &TM) {
  TargetLoweringObjectFileMachO::Initialize(Ctx, TM);
}

unsigned ARMMachOTargetObjectFile::getPersonalityEncoding() const {
    return DW_EH_PE_indirect | DW_EH_PE_pcrel | DW_EH_PE_sdata4;
}

unsigned ARMMachOTargetObjectFile::getLSDAEncoding() const {
    return DW_EH_PE_pcrel | DW_EH_PE_sdata4;
}

unsigned ARMMachOTargetObjectFile::getFDEEncoding() const {
    return DW_EH_PE_pcrel | DW_EH_PE_sdata4;
}

unsigned ARMMachOTargetObjectFile::getTTypeEncoding() const {
  return DW_EH_PE_indirect | DW_EH_PE_pcrel | DW_EH_PE_sdata4;
}


