# Standalone Handoff — M15.315: Z80-Safe VDP DMA Executor & HUD Portrait DMA (`0x002632..0x002678`, `0x0027A6..0x002896`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x002632..0x002678`: `hud_portrait_dma` (`$FF183E` vs `$FF183F`, table `0x000AA3EC + portrait * 0x60`, VRAM `0xF1C0`, `0x60` words)
  - `0x0027A6..0x0027FA`: `vdp_dma_exec_regs` (IRQs masked `0x0700`, Z80 busreq `$A11100`, VDP reg1 shadow `$FF0BF4` bit 4 DMA-enable)
  - `0x0027FC..0x002896`: `vdp_dma_exec_struct(a0)` (8-byte DMA descriptor: `+0` `uint32 src_word_addr`, `+4` `uint16 vram_dst`, `+6` `uint16 len_words`)
- **Evidence JSON:** `data/thor-evidence/rom_vdp_dma_exec_m15_315.json`
- **Python Extractor & Tests:** `tools/extract_rom_vdp_dma_exec.py`, `tests/rom_vdp_dma_exec_test.py`

## VDP DMA Register Sequence (`0x0027FC`)
1. `0x9400 | (len_words >> 8)`
2. `0x9300 | (len_words & 0xFF)`
3. `0x9500 | (src_word_addr & 0xFF)`
4. `0x9600 | ((src_word_addr >> 8) & 0xFF)`
5. `0x9700 | ((src_word_addr >> 16) & 0xFF)`
6. `(vram_dst & 0x3FFF) | 0x4000`
7. `((vram_dst >> 14) & 3) | 0x0080`
8. Restore VDP reg 1 shadow (`$FF0BF4`), re-issue command longword, and trigger DMA by writing `*(uint16*)(src_word_addr << 1)` to VDP data port (`0x00C00000`).
