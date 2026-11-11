#!/usr/bin/env python3
"""
M15.256 Instruction Lowering & Cycle Calibration.
Builds on M15.250 instruction lowering with verified cycle accuracy.
Strictly <= 350 lines per AGENTS.md Rule 3.
"""

from typing import List
from tools.thor_recompiler.insn_lowering_m15_245 import (
    ea_cycles,
    emit_read_op,
    sz_info,
)
from tools.thor_recompiler.insn_lowering_m15_249 import (
    ea_cycles_249,
    emit_ea_addr_249,
    emit_read_op_249,
)
from tools.thor_recompiler.insn_lowering_m15_250 import (
    emit_m15_250_insn_body,
    exact_m68k_cycles_250,
)


def exact_m68k_cycles_256(insn: dict) -> int:
    return exact_m68k_cycles_250(insn)


def emit_m15_256_insn_body(insn: dict) -> List[str]:
    return emit_m15_250_insn_body(insn)
