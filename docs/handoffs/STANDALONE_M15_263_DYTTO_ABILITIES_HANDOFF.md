# STANDALONE M15.263 DYTTO ABILITIES, STATE MACHINE & WATER DAMAGE HANDOFF

**Milestone:** SEGA-THOR HYBRID — M15.263  
**Repository:** `C:/Github/Sega-Thor`  
**Standalone Repository (`C:/Github/Sega-Thor-Standalone`):** STRICTLY READ-ONLY / UNTOUCHED  
**Machine-Readable Evidence Output:** `data/thor-evidence/rom_dytto_abilities_m15_263.json`  
**Extraction Tool:** `tools/extract_rom_dytto_abilities.py`  
**Deterministic Test Suite:** `tests/rom_dytto_abilities_test.py`

---

## 1. Complete Spirit State Machine (`0x014914`, 14 States `0x00..0x1A`)

Dispatched via `0x014908` using signed 16-bit self-relative offsets from `0x014914`:

| State | Handler | Role |
|---|---|---|
| `0x00` | `0x014932` | Autonomous Idle / PRNG Command & Wander Dispatch |
| `0x02` | `0x014930` | Immediate `RTS` (no-op) |
| `0x04` | `0x014A5C` | Follow Player (`0x0150A0`) + `$2(A6)` timer countdown -> `0x014932` (`State 0x00`) |
| `0x06` | `0x014ABC` | Magic Attack Stage 1 -> transitions to `State 0x08` on anim end (`$37` bit 1) |
| `0x08` | `0x014AEA` | Magic Attack Stage 2 (spawns `0x0024` via `0x015136`) -> `State 0x0A` |
| `0x0A` | `0x014B50` | Magic Attack Recovery -> `0x014958` (wander `State 0x04`) |
| `0x0C` | `0x014D1A` | Arcing Magic Projectile Flight (`vz += 0x6000`, collision `0x00B922`) |
| `0x0E` | `0x014EE6` | Knockback Deceleration (`vx/vy -= 0x8000`, `vz -= 0xA000`) |
| `0x10` | `0x014A72` | Dytto Special 3-Way Spread (`$30 == 0x0018` -> `0x015214`) |
| `0x12` | `0x014E1A` | Spread Projectile Flight & Hit Detection (`0x00B922` -> `0x014E86`) |
| `0x14` | `0x014E0C` | Projectile Pop / Despawn (`clr.w $0(A6)` on anim end) |
| `0x16` | `0x014B7C` | Return-to-Ali Approach (`$058CE6` overlap via `0x00BAAC`) -> `State 0x18` |
| `0x18` | `0x014BB8` | Ali Contact Buff / Callback (`$FF1940 = 0x014C04`, `jsr 0x00BCE2`) |
| `0x1A` | `0x014F60` | Awakening Cutscene Timeline (`180f..420f` -> `State 0x04`) |

---

## 2. Autonomous Command Dispatch (`State 0x00` at `0x014932`)

1. Tests cutscene lock `$FF1987`; if non-zero, jumps directly to `0x014958` (wander).
2. Calls PRNG `0x00E268` (`D0.W`):
   - `D0 <= 0x2000` (12.5%): `0x0149E0` -> `Action 0x02`, **`State 0x06`**, SFX `0x800251`.
   - `0x2000 < D0 <= 0x6000` (25.0%): `0x014A22` -> `Action 0x12`, **`State 0x10`** (Dytto 3-way spread).
   - `0x6000 < D0 <= 0x9000` (18.75%): `0x014A34` -> `Action 0x18`, **`State 0x16`**, speed `0x40000`, timer `0x18`, SFX `0x800251`.
   - `D0 > 0x9000` (43.75%): `0x014958` -> computes wander target halfway between `(256, 320)` and Ali (`$FF19E8`) plus PRNG jitter (`ASR.W #3`), sets adaptive speed (`0x20000` / `0x10000` / `0x8000`), timer `10` frames, **`State 0x04`**.

---

## 3. Dytto Special Attack & Water Damage (`0x015214`, `0x014E1A`, `0x014E86`)

- **3-Way Spread (`0x015214`):** Triggered from `State 0x10` (`0x014A8A`) only when `$30(A6) == 0x0018` (Dytto) and `$2E(A6) == 0`. Spawns 3 projectiles (`d6 = 0, +28, -28`) of Entity Type `0x0024` in **`State 0x12`**, `Action 0x14`, hitbox radius `5`, and plays SFX `0x80034F`.
- **Magic Burst (`0x015136`):** Triggered from `State 0x08` (`0x014AEA`) every 3rd frame (`$FF1971 % 3 == 0`) while `$32(A6) == 0x0C`. Spawns Entity Type `0x0024` in **`State 0x0C`**, `Action 0x06`, hitbox radius `8`, `vz = -0x00050000`.
- **Elemental Water Damage (`0x014E86`):** On collision via `0x00B922` (`-6..+6` XY, `-4..+6` Z), applies damage word **`0x2008`** (`$86(A0)`) to grounded targets or **`0xA008`** if airborne (`$10(A0) != 0`), where bit 13 (`0x2000`) is the Water Elemental flag and `0x08` is 8 base damage, plus SFX `0x700345`.
