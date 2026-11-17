#!/usr/bin/env python3
"""Unit tests for M15.315 ROM Z80-Safe VDP DMA Executor & HUD Portrait DMA."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_vdp_dma_exec import ROM_PATH, extract_evidence


def build_vdp_dma_struct_commands(src_word_addr: int, vram_dst: int, len_words: int) -> list[int]:
    """Reference encoding of VDP DMA control words from 0x0027FC..0x002882."""
    reg_94 = 0x9400 | ((len_words >> 8) & 0xFF)
    reg_93 = 0x9300 | (len_words & 0xFF)
    reg_95 = 0x9500 | (src_word_addr & 0xFF)
    reg_96 = 0x9600 | ((src_word_addr >> 8) & 0xFF)
    reg_97 = 0x9700 | ((src_word_addr >> 16) & 0xFF)
    cmd_hi = (vram_dst & 0x3FFF) | 0x4000
    cmd_lo = ((vram_dst >> 14) & 0x0003) | 0x0080
    return [reg_94, reg_93, reg_95, reg_96, reg_97, cmd_hi, cmd_lo]


def portrait_dma_src_word_addr(portrait_id: int) -> int:
    """Reference portrait word address: 0x0AA3EC + portrait_id * 0x60 (from 0x002650..0x002662)."""
    return 0x000AA3EC + ((portrait_id & 0xFF) * 0x60)


class TestRomVdpDmaExec(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["routines"]["hud_portrait_dma"]["vram_dst"], "0xF1C0")
        self.assertEqual(ev["routines"]["hud_portrait_dma"]["portrait_stride_bytes"], 96)
        saved = json.loads(
            Path("data/thor-evidence/rom_vdp_dma_exec_m15_315.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_dma_command_encoding_and_portrait_offset(self) -> None:
        src_word = portrait_dma_src_word_addr(2)
        self.assertEqual(src_word, 0x000AA3EC + 0xC0)
        cmds = build_vdp_dma_struct_commands(src_word, 0xF1C0, 0x0060)
        self.assertEqual(
            cmds,
            [
                0x9400,
                0x9360,
                0x95AC,
                0x96A4,
                0x970A,
                0x71C0,
                0x0083,
            ],
        )


if __name__ == "__main__":
    unittest.main()
