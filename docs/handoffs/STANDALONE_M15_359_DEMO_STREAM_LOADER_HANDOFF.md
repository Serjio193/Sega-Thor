# Standalone Handoff — M15.359: ROM Attract-Mode 9-Slot Demo-Stream Loader & Delta Decoder (`0x0089B2..0x008AA5`)

## ROM Source & Evidence
- ROM routine: `0x0089B2..0x008AA5`
- Evidence JSON: `data/thor-evidence/rom_demo_stream_loader_m15_359.json`
- Extractor: `tools/extract_rom_demo_stream_loader.py`
- Python tests: `tests/rom_demo_stream_loader_test.py`

## Exact Behavioral Contract
1. **Mode Guard (`0x0089B2..0x0089FA`)**:
   - Waits for V-Int sync bit 2 (`$FF164D`), resets entity/DMA/VDP state (`0x2D58`, `0x372A`, `0x376A`, `0x37BE`), resets stack (`$FF1930`).
   - If `$FF1996 == 0` (normal boot/title), jumps to `0x0004C6`; otherwise loads next attract-mode demo.
2. **9-Slot Demo Index & Decompression (`0x0089FE..0x008A2E`)**:
   - Increments `$FF0BFC` (`u8`); wraps to `0` if `> 8` (9 streams: `0..8`).
   - Indexes `0x008A94[idx]` (`u16` offset added to base `0x1698FE`), decompresses via `0x003820` into `$FFBFAC`.
3. **8KB Cumulative Delta Decode (`0x008A30..0x008A48`)**:
   - Decodes `0x2000` bytes (`0x1FFE` `dbra` iterations after initial byte) at `$FFDFB2`: `buf[i] = (buf[i-1] + buf[i]) & 0xFF`.
4. **Header & Replay Setup (`0x008A4A..0x008A90`)**:
   - Sets `$FF1996 = 0xFF`, reads 3 header words `(d0, d3, d4)` from `$FFBFAC`, stores replay stream pointer `$FF1998`, sets `$FF1928 = 0x7F28`, clears `$FF1971` and `$FF199C`, waits 1 V-Int (`$FF1656`), sets `$FF1997 = 0xFF`, and branches to `0x00CA24`.
