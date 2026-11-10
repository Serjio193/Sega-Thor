#!/usr/bin/env python3
"""
M15.249 Extended M68000 Decoder wrapper.
Adds support for instructions required by connected high-frequency hotpaths:
  - MOVE.<B|W|L> d8(An, Xn), <ea>
  - MOVE.<B|W|L> d8(PC, Xn), <ea>
  - <ORI|ANDI|SUBI|ADDI|CMPI>.<B|W|L> #imm, abs_l (0x0079..0x0C79)
  - MOVEA.L d8(PC, Xn), An (0x207B)
Strictly <= 200 lines per AGENTS.md Rule 3.
"""

import struct
from tools.thor_recompiler.decoder import DecodedInstruction, Operand
from tools.thor_recompiler.decoder_ext_m15_246 import decode_m15_246_at


def _disp8(ext: int) -> int:
    b = ext & 0xFF
    return b - 256 if b >= 128 else b


def decode_m15_249_at(rom: bytes, pc: int) -> DecodedInstruction:
    w0 = struct.unpack_from(">H", rom, pc)[0]

    # ADDQ / SUBQ to abs_l (0x5079..0x5F79)
    if (w0 & 0xF03F) == 0x5039:
        mn = "ADDQ" if (w0 & 0x0100) == 0 else "SUBQ"
        sz_bits = w0 & 0x00C0
        sz = "b" if sz_bits == 0 else ("w" if sz_bits == 0x0040 else "l")
        d_val = (w0 >> 9) & 7
        if d_val == 0: d_val = 8
        addr = struct.unpack_from(">I", rom, pc + 2)[0]
        words = [w0, (addr >> 16) & 0xFFFF, addr & 0xFFFF]
        return DecodedInstruction(pc, words, mn, sz,
                                  [Operand("imm", val=d_val, size=sz), Operand("abs_l", val=addr, size=sz)],
                                  base_cycles=12 if sz != "l" else 20)

    # Immediates to abs_l: <ORI|ANDI|SUBI|ADDI|CMPI>.<B|W|L> #imm, abs_l
    imm_types = {0x00: "ORI", 0x02: "ANDI", 0x04: "SUBI", 0x06: "ADDI", 0x0A: "EORI", 0x0C: "CMPI"}
    type_b = (w0 >> 8) & 0xFF
    if type_b in imm_types and (w0 & 0x003F) == 0x0039:
        sz_bits = w0 & 0x00C0
        sz = "b" if sz_bits == 0 else ("w" if sz_bits == 0x0040 else "l")
        mn = imm_types[type_b]
        words = [w0]
        offset = 2
        if sz == "l":
            imm = struct.unpack_from(">I", rom, pc + offset)[0]
            words.extend([(imm >> 16) & 0xFFFF, imm & 0xFFFF])
            offset += 4
        else:
            imm = struct.unpack_from(">H", rom, pc + offset)[0]
            if sz == "b": imm &= 0xFF
            words.append(imm)
            offset += 2
        addr = struct.unpack_from(">I", rom, pc + offset)[0]
        words.extend([(addr >> 16) & 0xFFFF, addr & 0xFFFF])
        base_cyc = 16 if sz != "l" else 24
        return DecodedInstruction(pc, words, mn, sz,
                                  [Operand("imm", val=imm, size=sz), Operand("abs_l", val=addr, size=sz)],
                                  base_cycles=base_cyc)

    # MOVE / MOVEA with src_mode == 6 (disp_ax) or src_mode == 7 & reg == 3 (pc_index)
    if (w0 & 0xC000) == 0x0000 and (w0 & 0x3000) != 0x0000:
        sz = "b" if (w0 & 0x3000) == 0x1000 else ("l" if (w0 & 0x3000) == 0x2000 else "w")
        dst_reg = (w0 >> 9) & 7
        dst_mode = (w0 >> 6) & 7
        src_mode = (w0 >> 3) & 7
        src_reg = w0 & 7

        if src_mode in (6, 7):
            words = [w0]
            offset = 2
            if src_mode == 6:
                ext = struct.unpack_from(">H", rom, pc + offset)[0]
                words.append(ext)
                offset += 2
                d8 = _disp8(ext)
                src_op = Operand("disp_ax", reg=src_reg, val=d8, ext=ext, size=sz)
            elif src_mode == 7 and src_reg == 3:
                ext = struct.unpack_from(">H", rom, pc + offset)[0]
                words.append(ext)
                offset += 2
                xn = (ext >> 12) & 7
                d8 = _disp8(ext)
                base_addr = pc + 2 + d8
                src_op = Operand("pc_index", reg=xn, val=base_addr, ext=ext, size=sz)
            else:
                return decode_m15_246_at(rom, pc)

            # Destination operand
            if dst_mode == 0:
                dst_op = Operand("reg_d", reg=dst_reg, size=sz)
            elif dst_mode == 1:
                return DecodedInstruction(pc, words, "MOVEA", sz,
                                          [src_op, Operand("reg_a", reg=dst_reg, size="l")],
                                          base_cycles=16 if sz == "l" else 12)
            elif dst_mode == 2:
                dst_op = Operand("indir_a", reg=dst_reg, size=sz)
            elif dst_mode == 3:
                dst_op = Operand("postinc_a", reg=dst_reg, size=sz)
            elif dst_mode == 5:
                disp16 = struct.unpack_from(">h", rom, pc + offset)[0]
                words.append(disp16 & 0xFFFF)
                dst_op = Operand("disp_a", reg=dst_reg, val=disp16, size=sz)
            elif dst_mode == 7 and dst_reg == 1:
                addr = struct.unpack_from(">I", rom, pc + offset)[0]
                words.extend([(addr >> 16) & 0xFFFF, addr & 0xFFFF])
                dst_op = Operand("abs_l", val=addr, size=sz)
            else:
                return decode_m15_246_at(rom, pc)

            base_cyc = 14 if sz != "l" else 18
            return DecodedInstruction(pc, words, "MOVE", sz, [src_op, dst_op], base_cycles=base_cyc)

    return decode_m15_246_at(rom, pc)
