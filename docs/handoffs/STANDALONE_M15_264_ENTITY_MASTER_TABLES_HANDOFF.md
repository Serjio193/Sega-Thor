# STANDALONE M15.264 ENTITY MASTER TABLES & ELEMENTAL DAMAGE HANDOFF

**Milestone:** SEGA-THOR HYBRID — M15.264  
**Repository:** `C:/Github/Sega-Thor`  
**Standalone Repository (`C:/Github/Sega-Thor-Standalone`):** STRICTLY READ-ONLY / UNTOUCHED  
**Machine-Readable Evidence Output:** `data/thor-evidence/rom_entity_master_tables_m15_264.json`  
**Extraction Tool:** `tools/extract_rom_entity_master_tables.py`  
**Deterministic Test Suite:** `tests/rom_entity_master_tables_test.py`

---

## 1. Entity Master Update Dispatch Table (`0x00DF54..0x00E0B8`)

- Contains **89** 32-bit big-endian handler pointers for even entity types `0x0016` through `0x00C6`, ending immediately before `RTS` (`0x4E75`) at `0x00E0B8`.
- Lookup formula: `handler_pc = u32_be(0x00DF54 - 0x2C + type_id * 2)`.
- Key verified handlers:
  - `0x0020` (Shrine Altar / Prop): `0x00E20A`
  - `0x0024` (Dytto Water Projectile): `0x014848`
  - `0x002A` (Chest / Static Prop): `0x00E0B8`
  - `0x002C` (Town Guard): `0x0254EA`
  - `0x002E` (Guard Projectile): `0x00E25C`
  - `0x0034` (Player Arrow): `0x00EA04`
  - `0x003C` (Village Elder NPC): `0x017D12`
  - `0x0048` (Charging Enemy): `0x01778C`
  - `0x004C` (Flame / Hazard): `0x016DCE`
  - `0x0060` (Town Archway): `0x0178EA`

---

## 2. 32-Byte Entity Descriptor Table (`0x03F2FA`, `0x008D06`)

- Indexed for all **99** even entity types `0x0002..0x00C6` via `addr = 0x03F2FA - 0x20 + (type_id << 4)`.
- Binary layout (32 bytes):
  - `+0x00` (`u16`): `$4A(A6)` Z-radius
  - `+0x02` (`u8`): `$46/$42/$44(A6)` XY-radius
  - `+0x03` (`u8`): `$48(A6)` `dim_x`
  - `+0x04` (`u8`): `$49(A6)` `dim_y`
  - `+0x05` (`u8`): `$3D(A6)` render priority
  - `+0x06` (`u8`): `$4C(A6)` mass / pushback
  - `+0x07..+0x0B` (`u8[5]`): flags `$3A`, `$36`, `$37`, `$38`, `$3C`
  - `+0x0C..+0x18` (`u32[4]`): animation/sprite table pointers `$1A`, `$1E`, `$26`, `$22`
  - `+0x1C` (`u32`): `$5E(A6)` AI/script pointer (for main entity pool `A6 < $FF2954`)

---

## 3. Universal Damage & Elemental Bit-13 Pipeline (`0x017E30`)

1. Reads and clears incoming damage word `$86(A6)`.
2. Tests **bit 13 (`0x2000`)** (`0x017E4A: btst.b #$d, d0`): if set (e.g. Dytto's `0x2008` / `0xA008`), sets `$39(A6)` bit 3 and plays SFX `0x8004D9` (`0x00E7E8`).
3. Masks magnitude with `0x0FFF` (`0x017E66: andi.w #$fff, d0`).
4. Subtracts entity defense `$92(A6)` (`0x017E6A: sub.w $92(a6), d0`), clamping minimum damage to `1` (`bhi; moveq #1, d0`).
5. Subtracts from HP `$8A(A6)` (clamped `>= 0`). When HP reaches `0`, transitions entity to death `State 2` (`0x008EF8`, timer `0x10`, `$38(A6)` bit 3).
