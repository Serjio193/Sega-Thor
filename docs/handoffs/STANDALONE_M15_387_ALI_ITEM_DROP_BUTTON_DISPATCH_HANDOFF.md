# Standalone Handoff — M15.387: ROM Ali Item Drop Spawner, 8-Slot Allocation Probe, Input Ring Sampler & Master Button Dispatcher (`0x007C3C..0x007E46`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x007C3C..0x007CC4`: `ali_item_drop_spawn` (allocates world entity type 8 for item drop: copies player coordinates `+8` and `+C`, installs 60-frame timer `+0x22 = 0x3C`, item ID `+0x32`, sub-type `+0x52`, state `+0x4`, sets bit 0 in `+0x39`, clears flags in `$FF17B8`, and transitions player to idle/walk `0x62CC`).
  - `0x007CC6..0x007D22`: `ali_item_drop_reject` (sets rejection state `state = 0x3C, anim = 0x44`, plays buzzer SFX `#$C00255`, clears `$FF1864`).
  - `0x007D24..0x007D88`: `ali_item_slot_probe` (scans 8 entity slots at `$FF2ABC` with stride 90 bytes `0x5A` against table `0x05CE16.l` and distance check `0xE5AE`).
  - `0x007D8A..0x007DC6`: `ali_input_ring_sample` (samples and ORs composite button masks across `d0` frames from input ring buffer at `$FF1668`, wrapping head at `0x7E`).
  - `0x007DC8..0x007E46`: `ali_master_button_dispatcher` (evaluates up to 5 recent frames; dispatches Jump on bit 6, Attack on bit 4, Latched attack on `$FF165E`, and Spirit power on bit 5).
- **Evidence JSON:** `data/thor-evidence/rom_ali_item_drop_button_dispatch_m15_387.json`
- **Python Extractor & Tests:** `tools/extract_rom_ali_item_drop_button_dispatch.py`, `tests/rom_ali_item_drop_button_dispatch_test.py`
