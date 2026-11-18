# STANDALONE HANDOFF: Milestone M15.458 — ROM Entity Pool 2 Knockback Stun Broadcast & Global Type Scanner

## 1. Context & Range
- **Milestone:** M15.458
- **ROM Addresses:** `0x00F1B2..0x00F258` (166 bytes, 44 instructions)
- **Target Subsystem:** Global Entity Utilities & Knockback Stun Broadcast Engine:
  * Pool 2 Entity Knockback / Stun Wave Broadcast & Cutscene Script Trigger (`0x00F1B2..0x00F234`, 130 bytes)
  * Global Entity Type ID Search Scanner across Pool 1 + Ali (`0x00F234..0x00F258`, 36 bytes)
- **Evidence JSON:** `data/thor-evidence/rom_entity_stun_lookup_m15_458.json`
- **Unit Tests:** `tests/rom_entity_stun_lookup_test.py`

---

## 2. Subroutines & ROM Architecture

### Subroutine 1: `Entity_Pool2_KnockbackStunWaveBroadcast` (`0x00F1B2..0x00F234`, 130 bytes, 34 insns)
- Called from script engines (e.g. `0x02C832`, `0x02FDDC`, `0x031870`).
- Input: `d0.w` = caller entity index within Pool 2 (`$FF1CD8`).
- Multiplies `d0` by 188 (`mulu.w #$BC, d0`) to resolve caller entity slot `a5 = $FF1CD8 + d0 * 188`.
- If slot `a5` is currently active (`tst.w $0(a5) != 0`):
  * Aborts execution and returns with Carry clear (`andi.b #$FE, ccr`).
- Scans all 17 slots of Pool 2 (`d1 = 16`, `dbra d1` loop):
  * Checks if candidate slot `a6` is active: `tst.w $0(a6) <= 0` -> skip.
  * Checks if candidate is caller slot: `cmpa.l a5, a6 == 0` -> skip.
  * Checks intangibility: `btst.b #3, $38(a6) != 0` -> skip.
  * Checks invulnerability: `btst.b #0, $3A(a6) != 0` -> skip.
  * Checks current hitstun: `tst.b $82(a6) != 0` -> skip.
  * Inverts horizontal facing: `d0 = +$16(a6); bchg #1, d0; +$84(a6) = d0`.
  * Clears `d7 = 0` and calls `0x00E4D0` to project directional knockback trajectory.
  * Inflicts heavy knockback damage: `+$86(a6) = 0xCFFF`.
  * Sets hit timer: if `+$8C(a6) == 0`, sets `+$8C(a6) = 0xFFFF`.
  * Advances to next slot: `lea $BC(a6), a6`.
- Sets cutscene / script event trigger longword at `$FF17B2`:
  `move.l #$0004382E, $FF17B2.l`.
- Returns with Carry flag set (`ori.b #$01, ccr`).

### Subroutine 2: `Entity_FindByTypeIdGlobal` (`0x00F234..0x00F258`, 36 bytes, 10 insns)
- Called from script and combat coordinator engines (e.g. `0x0300C2`, `0x0300DC`).
- Input: `d0.w` = target entity type ID (e.g. `#$3A` for Colossus boss, `#$1E` for cutscene entity).
- Scans 21 slots starting at `$FF19E8` (`d1 = 20`, `dbra d1`, stride 188 / `$BC`):
  * Tests if entity type matches: `cmp.w $0(a0), d0`.
  * If match found:
    - Returns `a0` pointing to the matching entity slot.
    - Sets Carry flag: `ori.b #$01, ccr`.
  * If loop exhausts without match:
    - Clears Carry flag: `andi.b #$FE, ccr`.

---

## 3. Key Constants
| Constant | Value | Description |
|---|---|---|
| Pool 2 Base RAM | `0x00FF1CD8` | Base address of Pool 2 (17 slots) |
| Pool 2 Capacity | `17` | Total entity slots in Pool 2 |
| Entity Stride | `188` (`0xBC`) | Size of one entity record in bytes |
| Pool 1 Base RAM | `0x00FF19E8` | Base address of Pool 1 (21 slots, includes Ali at slot 0) |
| Pool 1 Capacity | `21` | Total entity slots scanned by global lookup |
| Knockback Damage Flag | `0xCFFF` | Written to `+$86` (heavy knockback + 0xFF damage) |
| Hit Timer Flag | `0xFFFF` | Written to `+$8C` if previously 0 |
| Script Event Address | `0x00FF17B2` | Event trigger queue in work RAM |
| Script Event ID | `0x0004382E` | Knockback wave completion event code |
