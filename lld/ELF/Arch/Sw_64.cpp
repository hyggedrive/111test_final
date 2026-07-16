//===- Sw_64.cpp -----------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "InputFiles.h"
#include "OutputSections.h"
#include "Symbols.h"
#include "SyntheticSections.h"
#include "Target.h"
#include "Thunks.h"
#include "lld/Common/ErrorHandler.h"
#include "llvm/Object/ELF.h"
#include "llvm/Support/Endian.h"

using namespace llvm;
using namespace llvm::object;
using namespace llvm::ELF;
using namespace llvm::object;
using namespace llvm::support::endian;
using namespace lld;
using namespace lld::elf;

namespace {
template <class ELFT> class Sw_64 final : public TargetInfo {
public:
  Sw_64();
  uint32_t calcEFlags() const override;
  RelExpr getRelExpr(RelType type, const Symbol &s,
                     const uint8_t *loc) const override;
  RelType getDynRel(RelType type) const override;
  void writePltHeader(uint8_t *buf) const override;
  void writePlt(uint8_t *buf, const Symbol &sym,
                uint64_t pltEntryAddr) const override;
  void relocate(uint8_t *loc, const Relocation &rel,
                uint64_t val) const override;

private:
  static inline uint32_t high(uint32_t value){
    return (value >> 16) + (0 != (value & 0x8000));
  }

  static inline void Write16(void *P, uint16_t V){
    return write16<ELFT::TargetEndianness>(P, V);
  }

  static inline void Write32(void *P, uint32_t V){
    return write32<ELFT::TargetEndianness>(P, V);
  }

  static inline void Write64(void *P, uint64_t V){
    return write64<ELFT::TargetEndianness>(P, V);
  }

  static inline uint16_t Read16(void *P){
    return read16<ELFT::TargetEndianness>(P);
  }

  static inline uint32_t Read32(void *P){
    return read32<ELFT::TargetEndianness>(P);
  }

  static inline uint64_t Read64(void *P){
    return read64<ELFT::TargetEndianness>(P);
  }

  static inline uint32_t bit_select32(uint32_t a, uint32_t b, uint32_t mask){
    return (a & ~mask) | (b & mask);
  }

  static constexpr int32_t LITUSE_JSR = 3;
  static constexpr uintptr_t RA_SHIFT = 21;
  static constexpr uintptr_t RB_SHIFT = 16;
  static constexpr uintptr_t RC_MASK  = 31;
  static constexpr uintptr_t RA_MASK  = RC_MASK << RA_SHIFT;
  static constexpr uintptr_t RB_MASK  = RC_MASK << RB_SHIFT;

};
} // namespace

template <class ELFT>
Sw_64<ELFT>::Sw_64(){
  defaultCommonPageSize = 0x2000;
  defaultMaxPageSize = 0x10000;
  defaultImageBase = 0x120000000;
  gotRel = R_SW_64_GLOB_DAT;
  pltRel = R_SW_64_JMP_SLOT;
  relativeRel = R_SW_64_RELATIVE;
  symbolicRel = R_SW_64_REFQUAD;
  tlsGotRel = R_SW_64_TPREL64;
  tlsModuleIndexRel = R_SW_64_DTPMOD64;
  tlsOffsetRel = R_SW_64_DTPREL64;
  pltEntrySize = 0x4;
  pltHeaderSize = 0x24;

  // .got.plt header should be 16 bytes. We only get 8 bytes here.
  // A dummy entry will be added when necessary to compensate.
  gotPltHeaderEntriesNum = 1;

  // SW dynamic linker treat rela as rel and does not use addend in rela at all,
  // so we force to write out the addend to the position relocated.
  config->writeAddends = true;
}

template <class ELFT>
uint32_t Sw_64<ELFT>::calcEFlags() const {
  return /* sw6a */ 0x4;
}

template <class ELFT>
RelExpr Sw_64<ELFT>::getRelExpr(RelType Type, const Symbol &S, const uint8_t *Loc) const {
  switch(0xff & Type){
  case R_SW_64_NONE:
    return R_NONE;
  case R_SW_64_LITERAL:
    if(S.isFunc() && (LITUSE_JSR << 8) == (Type & 0xff00))return R_SW_PLT_GOT_OFF;
    // fallthrough
  case R_SW_64_GLOB_DAT:
  case R_SW_64_GOTTPREL:
    return R_SW_GOT_OFF;
  case R_SW_64_TLSGD:
  case R_SW_64_TLSLDM:
    return R_SW_TLSGD_GOT;
  case R_SW_64_GPDISP:
    return R_SW_GOT_GP_PC;
  case R_SW_64_LITUSE:
  case R_SW_64_HINT:
    return R_SW_HINT_PC;
  case R_SW_64_BRADDR:
  case R_SW_64_BRSGP:
  case R_SW_64_SREL16:
  case R_SW_64_SREL32:
  case R_SW_64_SREL64:
    return R_PC;
  case R_SW_64_GPREL32:
  case R_SW_64_GPREL16:
  case R_SW_64_GPRELHIGH:
  case R_SW_64_GPRELLOW:
    return R_SW_GOTREL;
  case R_SW_64_REFLONG:
  case R_SW_64_REFQUAD:
  case R_SW_64_LITERAL_GOT:
    return R_ABS;
  case R_SW_64_TPREL64:
  case R_SW_64_TPRELHI:
  case R_SW_64_TPRELLO:
  case R_SW_64_TPREL16:
    return R_TPREL;
  case R_SW_64_DTPRELHI:
  case R_SW_64_DTPRELLO:
    return R_DTPREL;
  default:
    error(getErrorLocation(Loc) + "do not know how to handle relocation " + toString(Type));
    return R_NONE;
  }
}



template <class ELFT> RelType Sw_64<ELFT>::getDynRel(RelType type) const {
    return type;
}

template <class ELFT>
void Sw_64<ELFT>::relocate(uint8_t *Loc, const Relocation &rel,
                          uint64_t Val) const {
  RelType Type = rel.type;
  switch(0xff & Type){
  case R_SW_64_NONE:
  case R_SW_64_LITUSE:
  case R_SW_64_LITERAL_GOT:
    // This can't be guarded by `literalgot` option, gcc already generates this
    // relocations pre-8X.

    // Nothing to do.
    break;
  case R_SW_64_GPDISP: {
    auto Addend = llvm::SignExtend32<24>(Type >> 8);
    auto i_hi = Read32(Loc);
    auto i_lo = Read32(Loc + Addend);

    auto implicit_addend = ((i_hi & 0xffff) << 16) | (i_lo & 0xffff);
    implicit_addend = (implicit_addend ^ 0x80008000) - 0x80008000;
    // Addend is for the address of the other instruction, not the displacement.
    Val -= Addend;
    Val += implicit_addend;
    if(static_cast<int64_t>(Val) < INT32_MIN || INT32_MAX < static_cast<int64_t>(Val)){
      error(getErrorLocation(Loc) + toString(Type) + " overflowed: 0x" + Twine::utohexstr(Val));
    }
    i_hi = bit_select32(i_hi, high(Val), 0xffff);
    i_lo = bit_select32(i_lo, Val, 0xffff);
    Write32(Loc, i_hi);
    Write32(Loc + Addend, i_lo);
  }break;
  case R_SW_64_BRSGP:
    // TODO: Check .prologue.
    // fallthrough
  case R_SW_64_BRADDR:
  {
    auto inst = Read32(Loc);
    Val = (Val >> 2) - 1;
    Write32(Loc, bit_select32(inst, Val, 0x1fffff));
  }break;
  case R_SW_64_HINT: {
    Val = (Val >> 2) - 1;
    auto inst = bit_select32(Read32(Loc), Val, 0xffff);
    Write32(Loc, inst);
  }break;
  case R_SW_64_SREL16: {
    // TODO: Overflow?
    auto inst = Read16(Loc);
    Write16(Loc, Val + inst);
  }break;
  case R_SW_64_REFLONG:
  case R_SW_64_SREL32:
  case R_SW_64_GPREL32:
  {
    // TODO: Overflow?
    auto inst = Read32(Loc);
    Write32(Loc, Val + inst);
  }break;
  case R_SW_64_DTPMOD64:
    Val = !config->shared;
    // fallthrough
  case R_SW_64_REFQUAD:
  case R_SW_64_SREL64:
  case R_SW_64_TPREL64:
  {
    // TODO: Overflow?
    auto inst = Read64(Loc);
    Write64(Loc, Val + inst);
  }break;
  case R_SW_64_GLOB_DAT:
  case R_SW_64_JMP_SLOT:
  {
    Write64(Loc, Val);
  }break;
  case R_SW_64_GPRELHIGH:
  case R_SW_64_TPRELHI:
  case R_SW_64_DTPRELHI:
  {
    auto inst = Read32(Loc);
    Write32(Loc, bit_select32(inst, high(Val), 0xffff));
  }break;
  case R_SW_64_LITERAL: {
    auto literal_inst = Read32(Loc);
    auto hi_addend = llvm::SignExtend32<16>(Type >> 16);
    if(0 != hi_addend){
      auto hi_loc = Loc + hi_addend;
      auto got_inst = Read32(hi_loc);
      // TODO: Check the protocol is correct.

      // Change previous instruction with R_SW_64_LITERAL_GOT to load the high part of GOT.
      Write32(hi_loc, bit_select32(got_inst, high(Val), 0xffff));
      // Change the base register of low load to the destination register of the high load.
      literal_inst = bit_select32(literal_inst, (got_inst & RA_MASK) >> (RA_SHIFT - RB_SHIFT), RC_MASK << RB_SHIFT);
    }else if(static_cast<int64_t>(Val) < INT16_MIN || INT16_MAX < static_cast<int64_t>(Val)){
      error(getErrorLocation(Loc) + toString(Type) + " overflowed: 0x" + Twine::utohexstr(Val));
    }
    Write32(Loc, bit_select32(literal_inst, Val, 0xffff));
  }break;

#define LDIH_GOT(Loc, Type, Val, option) \
    do{ \
      auto inst = Read32(Loc); \
      Val += inst & 0xffff; \
      \
      auto hi_addend = llvm::SignExtend32<16>(Type >> 16); \
      if(0 != hi_addend){ \
        if(config->option){ \
          auto hi_loc = Loc + hi_addend; \
          auto got_inst = Read32(hi_loc); \
          \
          /* Change previous instruction with R_SW_64_TLSREL_GOT to load the high part of GOT. */ \
          Write32(hi_loc, bit_select32(got_inst, high(Val), 0xffff)); \
          /* Change the base register of low load to the destination register of the high load. */ \
          inst = bit_select32(inst, (got_inst & RA_MASK) >> (RA_SHIFT - RB_SHIFT), RC_MASK << RB_SHIFT); \
        }else{ \
          error(getErrorLocation(Loc) + "add `--" #option "` to enable large range relocation " + toString(0xffff & Type)); \
        } \
      }else if(static_cast<int64_t>(Val) < INT16_MIN || INT16_MAX < static_cast<int64_t>(Val)){ \
        error(getErrorLocation(Loc) + toString(0xffff & Type) + " overflowed: 0x" + Twine::utohexstr(Val)); \
      } \
      Write32(Loc, bit_select32(inst, Val, 0xffff)); \
    }while(false) \
    // end of LDIH_GOT

  case R_SW_64_TLSGD:
    LDIH_GOT(Loc, Type, Val, sw64_tlsrelgot_tlsgd);
    break;
  case R_SW_64_TLSLDM:
    LDIH_GOT(Loc, Type, Val, sw64_tlsrelgot_tlsldm);
    break;
  case R_SW_64_GOTTPREL:
    LDIH_GOT(Loc, Type, Val, sw64_tlsrelgot_gottprel);
    break;
  case R_SW_64_GPRELLOW:
  case R_SW_64_GPREL16:
  case R_SW_64_TPRELLO:
  case R_SW_64_TPREL16:
  case R_SW_64_DTPRELLO:
  {
    auto inst = Read32(Loc);
    Val += inst & 0xffff;
    Write32(Loc, bit_select32(inst, Val, 0xffff));
  }break;
  default:
    error(getErrorLocation(Loc) + "do not know how to relocate " + toString(Type));
  }
}

static int32_t PltIndex;

template <class ELFT>
void Sw_64<ELFT>::writePltHeader(uint8_t *Buf) const {
  const uint8_t PltData[] = {
    0x39, 0x01, 0x7c, 0x43, // subl	$r27,$r28,$r25
    0x00, 0x00, 0x9c, 0xff, // ldih	$r28,0($r28)          <--- needs relocation.
    0x79, 0x01, 0x39, 0x43, // s4subl	$r25,$r25,$r25
    0x00, 0x00, 0x9c, 0xfb, // ldi	$r28,0($r28)          <--- needs relocation.
    0x00, 0x00, 0x7c, 0x8f, // ldl	$r27,0($r28)
    0x19, 0x01, 0x39, 0x43, // addl	$r25,$r25,$r25
    0x08, 0x00, 0x9c, 0x8f, // ldl	$r28,8($r28)
    0x00, 0x00, 0xfb, 0x0f, // jmp	$r31,($r27),1200020, 0x<_PROCEDURE_LINKAGE_TABLE_+0x20>
    0xf7, 0xff, 0x9f, 0x13, // br	$r28,1200000, 0x<_PROCEDURE_LINKAGE_TABLE_>
  };
  if(sizeof(PltData) != this->pltHeaderSize)error("Wrong PLT header data.");
  memcpy(Buf, PltData, sizeof(PltData));
  constexpr int Addend = 8;
  Relocation rel;
  rel.type = (R_SW_64_GPDISP | (Addend << 8));
  // $28 point to PLT[0], need to be relocated to the beginning of .got.plt section.
  auto Val = in.gotPlt->getVA() - in.plt->getVA() - sizeof(PltData) + Addend;
  relocate(Buf + 4, rel, Val);
  PltIndex = 0;
}

template <class ELFT>
void Sw_64<ELFT>::writePlt(uint8_t *buf, const Symbol &sym,
                          uint64_t pltEntryAddr) const {
  // `Index` is useless when multiple entries for the same function.
  Write32(buf, 0x13fffffe - PltIndex++);

}

template <class ELFT> TargetInfo *elf::getSw_64TargetInfo() {
  static Sw_64<ELFT> target;
  return &target;
}

template TargetInfo *elf::getSw_64TargetInfo<ELF64LE>();
