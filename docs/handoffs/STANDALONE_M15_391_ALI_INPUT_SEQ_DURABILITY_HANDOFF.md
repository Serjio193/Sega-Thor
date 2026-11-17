# Standalone Handoff — M15.391: ROM Ali Input Sequence Pattern Matcher, Shockwave Spawner, Weapon Durability & Break SFX (`0x0083F8..0x0085E2`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x0083F8..0x00846A`: `ali_motion_sequence_matcher` (stateful 3-stage recognizer: scans up to 12 frames in ring `$FF1668` for Press `(w & mask) == target` -> Neutral Release `(w & mask) == 0` -> Re-press `(w & mask) == target`; returns carry clear on success, carry set on failure).
  - `0x00846C..0x0084FA`: `ali_shockwave_projectile_spawner` (spawns shockwave projectile entity `0x16` at `$FF1AA4` with elevation 19, orientation offsets table `0x0084FC` `[0xF800, 0x0B00, 0x0817, 0xF500]`, and directional speeds `0x800` via `0xE2D4`).
  - `0x008504..0x00852E`: `ali_vdp_dma_enqueue` (writes VDP DMA commands `0x9D2FE`, `0x9F00`, `0x390` to ring at `$FF1892` with interrupt protection `SR = 0x0700`).
  - `0x008530..0x0085C2`: `ali_weapon_durability_break` (decrements active weapon durability at `$FF0D80 + slot*2 + 1`; ignores infinite ammo `0xFF`; on reaching 0, clears slot, reverts to default Dagger ID 1 / type `0x0A`, sets 90-frame toast timer `$FF1856 = 0x5A`, sets `$FF198F = 1`, and plays weapon break SFX `#$C00269`).
  - `0x0085C4..0x0085E2`: `ali_exp_bonus_check` (checks EXP threshold `$FF0E3C >= $FF1A70`; clears EXP and sets guaranteed bonus drop flag `$FF0DBB = 0xFF`).
- **Evidence JSON:** `data/thor-evidence/rom_ali_input_seq_durability_m15_391.json`
- **Python Extractor & Tests:** `tools/extract_rom_ali_input_seq_durability.py`, `tests/rom_ali_input_seq_durability_test.py`
