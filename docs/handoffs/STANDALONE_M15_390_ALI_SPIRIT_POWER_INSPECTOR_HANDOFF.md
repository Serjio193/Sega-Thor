# Standalone Handoff — M15.390: ROM Ali Spirit Power Invocation, Item Proximity Scanner, Object Inspector & 4-Direction Normalizer (`0x00822C..0x0083F8`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x00822C..0x0082AC`: `ali_spirit_power_invoke` (Spirit Power Button A logic: if moving and airborne `state == 4`, triggers Air Spirit Call `state = 0x16, anim = 0x16`, `vz = 0x00070000`, adds 8 to `$FF197E`; otherwise triggers Ground Spirit Summon `state = 0x10, anim = 0x10`, clears horizontal velocity, `vz = 0x00070000`, clears `$FF1980 = 0`, clears ring count `$FF16EA = 0`, unrolls stack via `$FF1934.l`).
  - `0x0082AE..0x0082F6`: `ali_item_proximity_scanner` (scans 12x12 box `[-6..+6, -6..+6]` for type 8 pickup entities; on hit, zeroes entity type `0(a0) = 0`, packs `(item_id << 8) | item_sub` into `$FF1976.l`, preserves initial state and trajectory, returns carry clear `ccr.C = 0`).
  - `0x0082F8..0x0083D2`: `ali_object_npc_inspector` (computes directional 4-way probe box in front of Ali, verifies alignment via angle routine `0xE338`; dispatches NPC Dialog `0x32 -> 0x18BBC`, Save Book `0x20 -> 0x18CAA`, Cutscene `0x94/0x96/0xBE -> 0x18EBE`, Chest Open `0xB0` with SFX `#$A00264`, sets chest anim 2 and Ali state `0x3F`).
  - `0x0083D4..0x0083F8`: `ali_facing_direction_normalizer` (maps facing direction indices: 0 -> 0, 1 -> 3, 2 -> 1, 3 -> 2).
- **Evidence JSON:** `data/thor-evidence/rom_ali_spirit_power_inspector_m15_390.json`
- **Python Extractor & Tests:** `tools/extract_rom_ali_spirit_power_inspector.py`, `tests/rom_ali_spirit_power_inspector_test.py`
