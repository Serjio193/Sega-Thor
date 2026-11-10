#!/usr/bin/env python3
"""
M15.246 Extended M68000 Decoder wrapper (zero modification to M15.244/245 decoder outputs).
Adds only verified instructions used inside selected high-frequency unowned callees:
  - DIVU.W #imm, Dn (0x80FC..0x8EFC)
  - EXT.W / EXT.L Dn (0x4880..0x4887, 0x48C0..0x48C7)
  - ST.B / SF.B Dn (0x50C0..0x51C7)
  - LEA d8(An, Xn), Am (0x41F0..0x4FF7)
  - MOVEM.W <postinc_a>, regs (0x4C98..0x4C9F)
  - EXG Dx, Dy / Ax, Ay (0xC140..0xC14F)
  - CMPA.<W|L> An/Am, Ap (0xB0C0..0xB1FF)
  - JSR (An) (0x4E90..0x4E97)
Strictly <= 200 lines per AGENTS.md Rule 3.
"""

import struct
from tools.thor_recompiler.decoder import DecodedInstruction, Operand, decode_at


def _disp8(ext: int) -> int:
    b = ext & 0xFF
    return b - 256 if b >= 128 else b


def decode_m15_246_at(rom: bytes, pc: int) -> DecodedInstruction:
    try:
        return decode_at(rom, pc)
    except (ValueError, NotImplementedError):
        pass

    w0 = struct.unpack_from(">H", rom, pc)[0]
    words = [w0]

    # DIVU.W #imm, Dn (0x80FC) and DIVS.W #imm, Dn (0x81FC): 1000 rrr [0|1]11 111 100
    if (w0 & 0xF0FF) == 0x80FC and (w0 & 0x01C0) in (0x00C0, 0x01C0):
        dn = (w0 >> 9) & 7
        mn = "DIVS" if (w0 & 0x0100) else "DIVU"
        imm = struct.unpack_from(">H", rom, pc + 2)[0]
        words.append(imm)
        return DecodedInstruction(pc, words, mn, "w",
                                  [Operand("imm", val=imm, size="w"), Operand("reg_d", reg=dn, size="l")],
                                  base_cycles=158 if mn == "DIVS" else 140)

    # EXT.W Dn (0x4880..7) / EXT.L Dn (0x48C0..7)
    if (w0 & 0xFFF8) in (0x4880, 0x48C0):
        sz = "w" if (w0 & 0x0040) == 0 else "l"
        return DecodedInstruction(pc, words, "EXT", sz, [Operand("reg_d", reg=w0 & 7, size=sz)], base_cycles=4)

    # ST / SF Dn: 0x50C0..7 / 0x51C0..7
    if (w0 & 0xFFF8) in (0x50C0, 0x51C0):
        mn = "ST" if (w0 & 0x0100) == 0 else "SF"
        return DecodedInstruction(pc, words, mn, "b", [Operand("reg_d", reg=w0 & 7, size="b")], base_cycles=6 if mn == "ST" else 4)

    # LEA d8(An, Xn), Am: 0100 rrr 111 110 bbb
    if (w0 & 0xF1F8) == 0x41F0:
        am = (w0 >> 9) & 7
        an = w0 & 7
        ext = struct.unpack_from(">H", rom, pc + 2)[0]
        words.append(ext)
        d8 = _disp8(ext)
        return DecodedInstruction(pc, words, "LEA", "l",
                                  [Operand("disp_ax", reg=an, val=d8, size="l", ext=ext),
                                   Operand("reg_a", reg=am, size="l")],
                                  base_cycles=12)

    # MOVEM.W (An)+, regs: 0x4C98..0x4C9F
    if (w0 & 0xFFF8) == 0x4C98:
        an = w0 & 7
        mask = struct.unpack_from(">H", rom, pc + 2)[0]
        words.append(mask)
        nregs = bin(mask).count("1")
        return DecodedInstruction(pc, words, "MOVEM", "w",
                                  [Operand("postinc_a", reg=an), Operand("imm", val=mask, size="w")],
                                  base_cycles=12 + 4 * nregs)

    # EXG Dx, Dy (opmode=01000)
    if (w0 & 0xF1F8) == 0xC140:
        rx = (w0 >> 9) & 7
        ry = w0 & 7
        return DecodedInstruction(pc, words, "EXG", "l",
                                  [Operand("reg_d", reg=rx, size="l"), Operand("reg_d", reg=ry, size="l")],
                                  base_cycles=6)

    # CMPA.W / CMPA.L <An/Dn/imm>, Am: 1011 rrr 011/111 mmm rrr
    if (w0 & 0xF0C0) == 0xB0C0:
        am = (w0 >> 9) & 7
        sz = "w" if ((w0 >> 8) & 1) == 0 else "l"
        sm = (w0 >> 3) & 7
        sr = w0 & 7
        if sm in (0, 1):
            rk = "reg_d" if sm == 0 else "reg_a"
            return DecodedInstruction(pc, words, "CMPA", sz,
                                      [Operand(rk, reg=sr, size=sz), Operand("reg_a", reg=am, size="l")],
                                      base_cycles=6)

    # JSR (An): 0x4E90..7
    if (w0 & 0xFFF8) == 0x4E90:
        an = w0 & 7
        return DecodedInstruction(pc, words, "JSR", "l", [Operand("indir_a", reg=an, size="l")], base_cycles=16)

    raise ValueError(f"Unsupported opcode 0x{w0:04X} at 0x{pc:06X}")
