# STANDALONE M15.270 SOUND DRIVER & COMMAND DISPATCH HANDOFF

**Milestone:** SEGA-THOR HYBRID — M15.270  
**Repository:** `C:/Github/Sega-Thor`  
**Standalone Repository (`C:/Github/Sega-Thor-Standalone`):** STRICTLY READ-ONLY / UNTOUCHED  
**Machine-Readable Evidence Output:** `data/thor-evidence/rom_sound_command_dispatch_m15_270.json`  
**Extraction Tool:** `tools/extract_rom_sound_command_dispatch.py`  
**Deterministic Test Suite:** `tests/rom_sound_command_dispatch_test.py`

---

## 1. Driver Entrypoints (`0x060000..0x06000C`)

- `0x060000` -> `0x060090` (Init / Tick)
- `0x060004` -> `0x06042A` (Enqueue Sound/BGM Command `d0.L`)
- `0x060008` -> `0x061232` (Z80/FM/PSG Hardware Tick)

## 2. Command Format (`d0.L` at `0x06042A`)

- **System Commands (`0x00..0x08` when `(d0 & 0xFF00) == 0`):**
  - `0x00` -> `0x0604E6`, `0x01` -> `0x060A8A`, `0x02` -> `0x06064C` (Fade/Stop BGM), `0x03..0x08` -> `0x06074E..0x060D10`.
- **Category Commands (`0x0100..0x0500`):**
  - `0x0100` -> `0x060B50` (BGM Track)
  - `0x0200` -> `0x06112E` (SFX Bank 2, priority in `d0[23:16]`)
  - `0x0300` -> `0x060F1C` (SFX Bank 3, e.g. `0x80034F`, `0x800399`)
  - `0x0400` -> `0x060F54` (SFX Bank 4, e.g. `0x8004D9`)
  - `0x0500` -> `0x060CDA` (Category 5)
