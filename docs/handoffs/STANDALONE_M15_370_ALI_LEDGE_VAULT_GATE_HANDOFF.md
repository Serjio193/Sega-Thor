# Standalone Handoff — M15.370: ROM Ali 4-Dir Ledge Vault Launch & 12-Frame Climb Gate (`0x005BA2..0x005C80`)

## ROM Source & Evidence
- ROM routine: `0x005BA2..0x005C80`
- Evidence JSON: `data/thor-evidence/rom_ali_ledge_vault_gate_m15_370.json`
- Extractor: `tools/extract_rom_ali_ledge_vault_gate.py`
- Python tests: `tests/rom_ali_ledge_vault_gate_test.py`

## Exact Behavioral Contract
1. **12-Frame Climb Hold Gate (`0x005C82..0x005CC6`)**:
   - Tests directional D-pad bit (`Up: 0, Down: 1, Left: 2, Right: 3`).
   - If released: resets `Ali.state = 0`, `Ali.anim = 0`, aborts to saved stack frame (`$FF1934`).
   - If held: checks cancel bit 13; if not cancel, increments `$FF1970`.
   - If `$FF1970 < 12`: sets `Ali.facing = target_facing`, aborts to saved frame (continues holding ledge).
   - Once `$FF1970 >= 12`: returns normally to trigger vault launch!
2. **Directional Launch Velocities (`0x005BA2..0x005C40`)**:
   - Up: `vx = 0`, `vy = -0x00026666`, `vz = 0x20000`, `z += $FF196E`.
   - Right: `vx = 0x0002B333`, `vy = 0`, `vz = 0x30000`, `z += $FF196E`, `vy_accum += $FF196E`.
   - Down: `vx = 0`, `vy = 0x00021999`, `vz = 0x30000`; if `$FF196E < 0x18`, `z += 4`, `vy_accum += 4`; if `$FF196E < 0x10`, `z += 3`, `vy_accum += 3`; plus `$FF196E` offsets.
   - Left: `vx = -0x0002B333`, `vy = 0`, `vz = 0x30000`, `z += $FF196E`, `vy_accum += $FF196E`.
3. **Common Vault Launch (`0x005C42..0x005C80`)**:
   - Clears `+0x9E = 0`, sets `anim = 6`, `state = 0x0E` (airborne ledge vault).
   - Adds launch `(vx, vy)` to accumulated `+0x72/+0x76`.
   - ORs `0x0D` into `+0x38` (invuln + ignore collision), clears `$FF198E = 0`.
   - Plays vault SFX `0x00800340` via `0x0060004`, clears input ring count `$FF16EA = 0`.
