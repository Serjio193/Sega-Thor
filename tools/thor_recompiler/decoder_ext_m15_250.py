#!/usr/bin/env python3
"""
M15.250 Extended M68000 Decoder wrapper.
Wraps M15.249 decoder for connected hotpaths, AI substates, and camera scroll tracking.
Strictly <= 200 lines per AGENTS.md Rule 3.
"""

import struct
from tools.thor_recompiler.decoder import DecodedInstruction, Operand
from tools.thor_recompiler.decoder_ext_m15_249 import decode_m15_249_at


def decode_m15_250_at(rom: bytes, pc: int) -> DecodedInstruction:
    w0 = struct.unpack_from(">H", rom, pc)[0]

    # Dynamic bit manipulation with register: BTST/BSET/BCLR/BCHG Dn, (An) (0x01D0, etc.)
    if (w0 & 0xF138) in (0x0110, 0x0150, 0x0190, 0x01D0):
        bit_op = (w0 >> 6) & 3
        mnemonic = {0: "BTST", 1: "BCHG", 2: "BCLR", 3: "BSET"}[bit_op]
        d_reg = (w0 >> 9) & 7
        dst_mode = (w0 >> 3) & 7
        dst_reg = w0 & 7
        if dst_mode == 2:
            words = [w0]
            return DecodedInstruction(
                pc, words, mnemonic, "b",
                [Operand("reg_d", reg=d_reg, size="b"), Operand("indir_a", reg=dst_reg, size="b")],
                base_cycles=12 if bit_op != 0 else 8
            )

    return decode_m15_249_at(rom, pc)
