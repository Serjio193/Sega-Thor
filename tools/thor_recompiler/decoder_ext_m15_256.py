#!/usr/bin/env python3
"""
M15.256 Extended M68000 Decoder wrapper.
Wraps M15.250 decoder, adding ADDA.W d16(An), An support for hotspot routines.
Strictly <= 200 lines per AGENTS.md Rule 3.
"""

import struct
from tools.thor_recompiler.decoder import DecodedInstruction, Operand
from tools.thor_recompiler.decoder_ext_m15_250 import decode_m15_250_at


def decode_m15_256_at(rom: bytes, pc: int) -> DecodedInstruction:
    w0 = struct.unpack_from(">H", rom, pc)[0]

    # ADDA.W d16(An), An: 1101 <An> 011 101 <An_src> (e.g. 0xD2E8, 0xD0E8)
    if (w0 & 0xF1F8) == 0xD0E8:
        dst_a = (w0 >> 9) & 7
        src_a = w0 & 7
        disp16 = struct.unpack_from(">h", rom, pc + 2)[0]
        words = [w0, struct.unpack_from(">H", rom, pc + 2)[0]]
        return DecodedInstruction(
            pc, words, "ADDA", "w",
            [Operand("disp_a", reg=src_a, val=disp16, size="w"), Operand("reg_a", reg=dst_a, size="w")],
            base_cycles=12
        )

    return decode_m15_250_at(rom, pc)
