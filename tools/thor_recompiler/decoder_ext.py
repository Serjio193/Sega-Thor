#!/usr/bin/env python3
"""
Extended M68000 Decoder rules for M15.245 entity handler & helper instructions:
  - d8(An, Xn.w/.l) indexed EA helper (mode=6)
  - BTST #imm, d8(An, Xn) (0x0830)
  - OR.<b|w|l> <ea>, Dn  (0x8000.. with opmode 0,1,2)
  - AND.<b|w|l> <ea>, Dn (0xC000.. with opmode 0,1,2)
  - CMP.<b|w|l> <ea>, Dn (0xB000.. with opmode 0,1,2)
  - JMP abs.l (0x4EF9)
Strictly <= 500 lines per AGENTS.md Rule 3.
"""

import struct
from typing import List, Optional, Tuple


def _ext_disp8(ext: int) -> int:
    b = ext & 0xFF
    return b - 256 if b >= 128 else b


def _decode_ea_ext(rom: bytes, pc_ext: int, mode: int, reg: int, sz: str,
                   Operand, words: List[int]) -> Optional[Tuple[object, int, int]]:
    if mode == 0:
        return Operand("reg_d", reg=reg, size=sz), 0, 0
    if mode == 1 and sz != "b":
        return Operand("reg_a", reg=reg, size=sz), 0, 0
    if mode == 2:
        return Operand("indir_a", reg=reg, size=sz), 0, 4 if sz != "l" else 8
    if mode == 5:
        d16 = struct.unpack_from(">h", rom, pc_ext)[0]
        words.append(d16 & 0xFFFF)
        return Operand("disp_a", reg=reg, val=d16, size=sz), 2, 8 if sz != "l" else 12
    if mode == 6:
        ext = struct.unpack_from(">H", rom, pc_ext)[0]
        words.append(ext)
        d8 = _ext_disp8(ext)
        return Operand("disp_ax", reg=reg, val=d8, size=sz, ext=ext), 2, 10 if sz != "l" else 14
    if mode == 7 and reg == 1:
        addr = struct.unpack_from(">I", rom, pc_ext)[0]
        words.extend([(addr >> 16) & 0xFFFF, addr & 0xFFFF])
        return Operand("abs_l", val=addr, size=sz), 4, 12 if sz != "l" else 16
    if mode == 7 and reg == 4:
        if sz == "l":
            imm = struct.unpack_from(">I", rom, pc_ext)[0]
            words.extend([(imm >> 16) & 0xFFFF, imm & 0xFFFF])
            return Operand("imm", val=imm, size="l"), 4, 8
        imm16 = struct.unpack_from(">H", rom, pc_ext)[0]
        words.append(imm16)
        v = (imm16 & 0xFF) if sz == "b" else imm16
        return Operand("imm", val=v, size=sz), 2, 4
    return None


def decode_ext_at(rom: bytes, pc: int, DecodedInstruction):
    # Import Operand dynamically from DecodedInstruction's module to avoid circular imports
    import sys
    dec_mod = sys.modules[DecodedInstruction.__module__]
    Operand = dec_mod.Operand

    w0 = struct.unpack_from(">H", rom, pc)[0]
    words = [w0]

    # BTST #imm, <ea> with indexed/indirect modes: 0x0800..0x083F
    if (w0 & 0xFFC0) == 0x0800:
        bitnum = struct.unpack_from(">H", rom, pc + 2)[0] & 0xFF
        words.append(bitnum)
        mode = (w0 >> 3) & 7
        reg = w0 & 7
        if mode == 6:
            ext = struct.unpack_from(">H", rom, pc + 4)[0]
            words.append(ext)
            d8 = _ext_disp8(ext)
            return DecodedInstruction(pc, words, "BTST", "b",
                                      [Operand("imm", val=bitnum, size="b"),
                                       Operand("disp_ax", reg=reg, val=d8, size="b", ext=ext)],
                                      base_cycles=20)

    # JMP abs.l: 0x4EF9
    if w0 == 0x4EF9:
        tgt = struct.unpack_from(">I", rom, pc + 2)[0]
        words.extend([(tgt >> 16) & 0xFFFF, tgt & 0xFFFF])
        return DecodedInstruction(pc, words, "JMP", "l", [Operand("abs_l", val=tgt, size="l")],
                                  branch_target=tgt, base_cycles=12)

    # OR.<sz> <ea>, Dn: 0x8000.. (opmode 0,1,2)
    # CMP.<sz> <ea>, Dn: 0xB000.. (opmode 0,1,2)
    # AND.<sz> <ea>, Dn: 0xC000.. (opmode 0,1,2)
    top = w0 & 0xF000
    opmode = (w0 >> 6) & 7
    if top in (0x8000, 0xB000, 0xC000) and opmode in (0, 1, 2):
        # Skip MUL/DIV (already handled in decoder.py when opmode in (3,7))
        mn = {0x8000: "OR", 0xB000: "CMP", 0xC000: "AND"}[top]
        sz = ("b", "w", "l")[opmode]
        d_dst = (w0 >> 9) & 7
        s_mode = (w0 >> 3) & 7
        s_reg = w0 & 7
        parsed = _decode_ea_ext(rom, pc + 2, s_mode, s_reg, sz, Operand, words)
        if parsed is not None:
            op_src, _, ea_cyc = parsed
            base = 6 if (sz == "l" and s_mode in (0, 1, 7) and mn != "OR") else (4 + ea_cyc)
            if sz == "l" and s_mode == 0:
                base = 6
            return DecodedInstruction(pc, words, mn, sz, [op_src, Operand("reg_d", reg=d_dst, size=sz)],
                                      base_cycles=base)

    return None
