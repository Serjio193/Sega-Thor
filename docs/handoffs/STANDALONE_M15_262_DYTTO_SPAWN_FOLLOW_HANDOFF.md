# STANDALONE M15.262 DYTTO SPAWN, ANIMATION & FOLLOW MECHANICS HANDOFF

**Milestone:** SEGA-THOR HYBRID — M15.262  
**Repository:** `C:/Github/Sega-Thor`  
**Baseline Git Commit:** `219f13e99e0bdd59ab70be116b38b06662ef95ed`  
**Standalone Repository (`C:/Github/Sega-Thor-Standalone`):** STRICTLY READ-ONLY / UNTOUCHED  
**Machine-Readable Evidence Output:** `data/thor-evidence/rom_dytto_spawn_follow_m15_262.json`  
**Extraction Tool:** `tools/extract_rom_dytto_spawn_follow.py`  
**Deterministic Test Suite:** `tests/rom_dytto_spawn_follow_test.py`

---

## 1. Dytto Spawn Routine (`0x014668..0x0146BE`)

### 1.1 Signature & Register Inputs
- **Caller:** `0x018D1E` (Water Shrine Altar Event).
- **Register Inputs:**
  - `A6`: Base pointer to Player Entity (`$FF1AA4`).
  - `D0.L`: Spawn X coordinate (world space).
  - `D1.L`: Spawn Y coordinate (world space).
  - `D2.L`: Spawn Z elevation (world space + 16px).
  - `D3.W`: Facing angle.

### 1.2 Step-by-Step Execution
1. `MOVE.W #$0018, $00(A6)`: Sets entity type to `0x0018` (Dytto Spirit).
2. `JSR $008D06`: Core entity pool allocator and slot linker.
3. `MOVE.W #$0002, $16(A6)`: Sets collision layer/flags.
4. `MOVE.W #$04F8, $18(A6)`: Initial base VRAM pattern descriptor.
5. `MOVE.W #$0064, $88(A6)` / `MOVE.W #$0064, $8A(A6)`: Max SP = 100, Initial SP = 100.
6. `MOVE.W #$000A, $8C(A6)` / `MOVE.W #$000A, $8E(A6)`: SP consumption drain timer and step.
7. `MOVE.L D0, $08(A6)` / `MOVE.L D1, $0C(A6)` / `MOVE.L D2, $10(A6)`: Copies world coordinates.
8. `MOVE.W D3, $14(A6)`: Sets initial facing direction.
9. `MOVE.W #$0002, $04(A6)`: Default state (before altar override to `0x1A`).
10. `MOVE.W #$0004, $2A(A6)`: Default action (before altar override to `0x22`).
11. `BSR.W $014522`: Invokes graphics DMA queue loader.
12. `SUBQ.B #1, $FF19A7.L`: Decrements active spirit reservation counter.

---

## 2. Graphics & VRAM DMA Allocation (`0x01454C`)

- **ROM Source Address:** `0x00141580` (Nemesis compressed sprite graphics for Dytto).
- **Decompression Work Buffer:** `0x00FF2FA8` (decompressed via `0x003820`).
- **Target VRAM Tile:** `0x04D0` (byte address `0x9A00`).
- **DMA Queue Publication:** Emits `$007F97D4` command to DMA queue at `$FF1892`.

---

## 3. Spirit State Machine & Awakening Timeline (State `0x1A`)

The spirit state machine dispatches from `0x014908` using table `0x014914`.
When awakened at the altar, Dytto is placed into **State `0x1A` (APPEAR)** with **Action `0x22`**.

Handler `0x014F60` tracks sub-frame timer `$02(A6)`:
| Frame Count | Hex Value | Observable Engine Behavior |
|---|---|---|
| **180 frames** (3.0s) | `0x00B4` | Appearance particle burst and sound effect command invocation. |
| **240 frames** (4.0s) | `0x00F0` | Visual palette flash / alpha pulse effect. |
| **300 frames** (5.0s) | `0x012C` | Dialogue completion handoff (`0x00C2EC` finished). |
| **360 frames** (6.0s) | `0x0168` | Camera unfreezes, returns tracking to Player. |
| **420 frames** (7.0s) | `0x01A4` | Cutscene completion: sets `$FF164F`, clears `$FF1CD8`, transitions to normal **Follow State `0x04`**. |

---

## 4. Dytto Follow-Player Steering & Physics (`0x0150A0..0x015112`)

During normal follow behavior (State `0x04` / Handler `0x014A5C`), Dytto continuously tracks Ali:
1. **Target Angle Calculation:**
   - Calls `0x00E6F0` with `dx = Player.X - Dytto.X`, `dy = Player.Y - Dytto.Y`.
   - Returns binary angle `(0..255)`.
2. **Angular Steering:**
   - Compares target angle with current angle at `$AE(A6)`.
   - Steps angle toward target at fixed rate: **8 units per frame** (`MOVEQ #8, D1`).
   - Stores smoothed angle at `$AE(A6)`.
3. **Speed & Damping:**
   - Base velocity stored in `$B0(A6)`.
   - Damped with arithmetic right shift: `ASR.L #7, D6`.
   - Integrates position into `$08(A6)` (X) and `$0C(A6)` (Y).
   - Floats above player with target offset `$A6(A6)` and `$AA(A6)`.

---

## 5. Active Spirit Flag (`$FF0D7A`)

- `bit 0`: Dytto (Water)
- `bit 1`: Efreet (Fire)
- `bit 2`: Shade (Shadow)
- `bit 3`: Bow (Plant)

The engine checks `$FF0D7A` bit 0 to verify Dytto is available for summoning and controls HUD icon display.

---

## 6. Recommended C++20 Interface for Standalone Engine

```cpp
namespace oasis::game::spirit {

struct DyttoEntityConfig {
    static constexpr std::uint16_t kEntityType = 0x0018;
    static constexpr std::uint16_t kVramTileBase = 0x04D0;
    static constexpr std::uint32_t kRomGfxSource = 0x00141580;
    static constexpr std::uint16_t kMaxSp = 100;
    static constexpr std::uint16_t kSpDrainRate = 10;
    static constexpr std::uint8_t  kAngularTurnStep = 8;
};

class DyttoSpiritController {
public:
    enum class State : std::uint16_t {
        Idle       = 0x0000,
        AppearCutscene = 0x001A,
        FollowPlayer   = 0x0004,
        MagicAttack    = 0x0008,
    };

    void init_spawn(int32_t spawn_x, int32_t spawn_y, int32_t spawn_z);
    void update_frame(const PlayerState& player);

private:
    State state_{State::AppearCutscene};
    uint16_t appear_timer_{0};
    uint16_t current_angle_{0};
    uint16_t current_sp_{100};
};

} // namespace oasis::game::spirit
```
