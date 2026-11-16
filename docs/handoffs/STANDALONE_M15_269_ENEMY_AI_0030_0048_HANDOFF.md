# STANDALONE M15.269 ENEMY AI CONTRACTS (0x0048 CHARGER & 0x0030 GATE BOSS) HANDOFF

**Milestone:** SEGA-THOR HYBRID — M15.269  
**Repository:** `C:/Github/Sega-Thor`  
**Standalone Repository (`C:/Github/Sega-Thor-Standalone`):** STRICTLY READ-ONLY / UNTOUCHED  
**Machine-Readable Evidence Output:** `data/thor-evidence/rom_enemy_ai_0030_0048_m15_269.json`  
**Extraction Tool:** `tools/extract_rom_enemy_ai_0030_0048.py`  
**Deterministic Test Suite:** `tests/rom_enemy_ai_0030_0048_test.py`

---

## 1. Entity `0x0048` Charger (`0x01778C..0x0178E8`, Substate `$A6(A6)`)

- **Substate 0 (`0x0177A6`):** Saves home `(x, y)` in `$A8/$AA`, sets `action = 0`, transitions to `1`.
- **Substate 1 (`0x0177C6`):** Checks directional tile AABB table `0x01783E` (`0x00C1CE`):
  - Facing 0: `[-1, -24 .. +1, -1]`
  - Facing 1: `[+1, -1 .. +24, +1]`
  - Facing 2: `[-1, +1 .. +1, +24]`
  - Facing 3: `[-24, -1 .. -1, +1]`
  On hit, initializes velocity (`0x00E2D4`), `action = 2`, timer `$9E`, SFX `0x800399`, transitions to `2`.
- **Substate 2 (`0x01784E`):** Accelerates for first half of `$9E`, decelerates for second half, then flips facing (`bchg #1, $17`) and transitions to `3`.
- **Substate 3 (`0x0178B2`):** Returns to home `(x, y)` (`0x00E45E`), flips facing, transitions to `1`.

## 2. Entity `0x0030` Gate Boss (`0x015AE2..0x015B9C`, Substate `$66(A6)`)

- Spawns 5 linked child slots (`+$BC` stride) via `0x00E66E` with `action 0x3A`/`0x3C` and VRAM base `0x4200` for Room `0x0105` (Waterfall Gate Arena).
