# Standalone Handoff — M15.317: Dialogue Cursor & Border SAT Builder (`0x00C756..0x00C84C`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x00C756..0x00C81A`: `build_dialogue_sat`
  - `0x00C81C..0x00C836`: `emit_8_sprite_bar` (`x = 0xA0`, step `+0x20`, `8` sprites, `tile += 4`)
  - `0x00C838..0x00C84C`: `append_sat_entry` (caps at `0x50` = 80 sprites)
- **Evidence JSON:** `data/thor-evidence/rom_dialogue_sat_builder_m15_317.json`
- **Python Extractor & Tests:** `tools/extract_rom_dialogue_sat_builder.py`, `tests/rom_dialogue_sat_builder_test.py`

## Behavior
1. If `$FF1886 != 0` and `($FF1657 & 0x10) != 0`:
   - Emit cursor sprite at `X = 0x011C`, `Y = 0x014C - 12 * (3 - min(choice_row, 3))`, `tile = 0x86DC | ($FF1887 ? 0x1000 : 0)`, `size_link = 0x0000 + next_index`.
2. Emit four 8-sprite horizontal bars (`size_link = 0x0C00 + next_index`, `y = bar_y + 0x0110`, `tile = (bar_vram >> 5) | 0x8000` incrementing by `4` per sprite):
   - Two bars for `$FF1872` (`$FF1876`, `$FF1878`), two bars for `$FF1874` (`$FF187A`, `$FF187C`).
