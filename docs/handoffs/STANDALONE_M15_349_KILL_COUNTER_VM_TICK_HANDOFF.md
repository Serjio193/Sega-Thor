# Standalone Handoff — M15.349: Pickup Overflow Reclaimer, Kill-Counter & Event VM Wait-Tick (`0x00F64C..0x00F7CE`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x00F64C..0x00F6E0`: `reclaim_pickup_slot` (8 slots at `$FF2ABC`, VRAM `0x0694 + 9*i`)
  - `0x00F740..0x00F786`: `update_kill_counter_and_bonus_drop` (`$FF0DBB` -> `0x1101`, `$FF1A7C` `u16` saturating kill counter)
  - `0x00F788..0x00F7CE`: `step_event_vm_tick` (pause bit `$FF17B8 & 2`, `$FF17B6` wait-timer decrement before VM dispatch `0x00F7D0`)
- **Evidence JSON:** `data/thor-evidence/rom_kill_counter_vm_tick_m15_349.json`
- **Python Extractor & Tests:** `tools/extract_rom_kill_counter_vm_tick.py`, `tests/rom_kill_counter_vm_tick_test.py`
