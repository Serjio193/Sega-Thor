# STANDALONE HANDOFF: Milestone M15.526 — ROM Town NPC Spawn Initializer (20 Variants, `0x018072..0x018538`)

## 1. Subsystem Overview
- **ROM Address Range:** `0x018072..0x018538` (1222 bytes total: 274 M68K instructions across 1 master routine with 20 variant configuration blocks).
- **Core Responsibilities:**
  - **Fast-Path Dispatch (`0x018072..0x0180F2`):**
    - Reads spawn variant index `d0 = +$66(a6)`.
    - If `+$66 == 0`, branches directly to active NPC runtime tick `0x018538`.
    - For `+$66` in `1..20`, dispatches via a 20-entry `subq.b #1, d0; beq.w` chain (out-of-range values `> 20` fall through to Variant 1 at `0x0180F2`).
  - **Story Flag Gating:**
    - **Condition A (`($FF0DBF & 1) != 0 || ($FF0DBC & 1) == 0`):** Pre-event / alternate town state used by Variants `1, 2, 3, 4, 8, 10, 11`.
    - **Condition B (`($FF0DBF & 1) == 0 && ($FF0DBC & 1) != 0`):** Post-event town state used by Variants `1, 2, 3, 4, 8, 10, 11`.
    - **3-Stage Progression (Variant 5):** Uses `$FF0DBF:0` and `$FF0DC5:3` to select dialog `$05`, `$59`, or `$5D`.
  - **20 Variant Specifications:**
    | Variant (`+$66`) | ROM Range | Height `+$4A` | Anim `+$2A` | Flags / Facing | Patrol Box A `(xmin, ymin, xmax, ymax)` | Dialog A `+$A0` | Patrol Box B `(xmin, ymin, xmax, ymax)` | Dialog B `+$A0` |
    | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
    | **1** | `0x0180F2..0x01814A` | `0x14` (20) | `0x06` | — | `(0x00E8, 0x01A8, 0x01A8, 0x0210)` | `0x0001` | `(0x0048, 0x01C0, 0x0160, 0x02C0)` | `0x0055` |
    | **2** | `0x0181A2..0x0181F4` | — | `0x0A` | — | `(0x0040, 0x0188, 0x0098, 0x01A8)` | `0x0002` | `(0x0050, 0x0188, 0x0138, 0x01E0)` | `0x0056` |
    | **3** | `0x0181F4..0x018246` | — | `0x0E` | — | `(0x0070, 0x0288, 0x0120, 0x02E0)` | `0x0003` | `(0x0020, 0x01D8, 0x0178, 0x02E0)` | `0x0057` |
    | **4** | `0x018298..0x0182EA` | — | `0x12` | — | `(0x00C8, 0x01A8, 0x00E8, 0x0240)` | `0x0004` | `(0x00D0, 0x00F8, 0x0170, 0x01F0)` | `0x0058` |
    | **5** | `0x0182EA..0x018330` | `0x10` (16) | `0x14` | `+$3C:2, +$37:6/7` | — | `0x05 / 0x59 / 0x5D` | — | — |
    | **6** | `0x018330..0x018358` | `0x16` (22) | `0x16` | `+$37:6/7, +$16=3` | — | `0x0006` | — | — |
    | **7** | `0x018358..0x01837C` | — | `0x1E` | `+$38:2/0, +$36:2, +$3D=0` | — | `0x0000` | — | — |
    | **8** | `0x01837C..0x0183CC` | `0x08` (8) | `0x20` | — | `(0x00C8, 0x01D0, 0x01A0, 0x0228)` | `0x0000` | `(0x0020, 0x00C0, 0x0170, 0x0238)` | `0x0000` |
    | **9** | `0x0183CC..0x0183EC` | — | `0x28` | `z=0x50, +$38:2, +$36:3` | — | `0x0000` | — | — |
    | **10** | `0x01814A..0x0181A2` | `0x14` (20) | `0x06` | — | `(0x00E0, 0x01D0, 0x0168, 0x02A8)` | `0x000A` | `(0x0010, 0x0088, 0x0130, 0x0168)` | `0x005B` |
    | **11** | `0x018246..0x018298` | — | `0x0E` | — | `(0x00D0, 0x0100, 0x0138, 0x01A0)` | `0x000B` | `(0x00D0, 0x0088, 0x01D8, 0x01E0)` | `0x005C` |
    | **12** | `0x0183EC..0x018416` | `0x14` (20) | `0x06` | — | `(0x00B8, 0x0218, 0x0150, 0x03F0)` | `0x0029` | — | — |
    | **13** | `0x018416..0x018440` | `0x14` (20) | `0x06` | — | `(0x0070, 0x0320, 0x0190, 0x03F0)` | `0x0029` | — | — |
    | **14** | `0x018440..0x018464` | — | `0x12` | — | `(0x0038, 0x0208, 0x0118, 0x02C8)` | `0x002D` | — | — |
    | **15** | `0x018464..0x018488` | — | `0x0E` | — | `(0x00D0, 0x02E8, 0x0190, 0x03F0)` | `0x002C` | — | — |
    | **16** | `0x018488..0x0184AC` | — | `0x0A` | — | `(0x00C8, 0x0218, 0x0138, 0x03C0)` | `0x002B` | — | — |
    | **17** | `0x0184AC..0x0184D0` | — | `0x0A` | — | `(0x0118, 0x0210, 0x01B0, 0x0268)` | `0x002A` | — | — |
    | **18** | `0x0184D0..0x0184EC` | — | `0x0A` | `+$16=2, +$37:6` | — | `0x0031` | — | — |
    | **19** | `0x0184EC..0x018508` | — | `0x0E` | `+$16=2, +$37:6` | — | `0x0032` | — | — |
    | **20** | `0x018508..0x018526` | `0x14` (20) | `0x06` | `+$16=2, +$37:6` | — | `0x0033` | — | — |
  - **Common Epilogue (`0x018526..0x018538`):**
    - Copies original variant index `+$66` into `+$9E` (`move.w d0, $9e(a6)`).
    - Clears `+$66 = 0` (`clr.b $66(a6)`) and timer `+$2 = 0` (`clr.w $2(a6)`).
    - Falls through into `0x018538` (Active NPC Tick).

## 2. ROM Evidence Reference
- **Evidence JSON:** `data/thor-evidence/rom_town_npc_init_m15_526.json`
- **Extractor Tool:** `tools/extract_rom_town_npc_init.py`
- **Test Suite:** `tests/rom_town_npc_init_test.py` (8 tests covering all 20 variants, 8/8 PASS)

## 3. C++20 Cleanroom Interface Specification

```cpp
#pragma once
#include <cstdint>

namespace Thor::TownNPC {

struct PatrolBoundingBox {
    uint16_t xmin{0};
    uint16_t ymin{0};
    uint16_t xmax{0};
    uint16_t ymax{0};
};

struct TownNPCStoryFlags {
    bool flag_ff0dbf_bit0{false};
    bool flag_ff0dbc_bit0{false};
    bool flag_ff0dc5_bit3{false};
};

struct TownNPCEntity {
    uint8_t  submode_66{0};
    uint16_t saved_variant_9e{0};
    uint16_t timer_2{0};
    uint16_t height_4a{0};
    uint16_t anim_2a{0};
    uint16_t facing_16{0};
    uint16_t dialog_id_a0{0};
    uint8_t  flag_36{0};
    uint8_t  flag_37{0};
    uint8_t  flag_38{0};
    uint8_t  flag_3c{0};
    uint8_t  shadow_3d{1};
    int32_t  z_10{0};
    PatrolBoundingBox patrol_box{};
};

inline bool InitializeTownNPCSpawn(
    TownNPCEntity& npc,
    const TownNPCStoryFlags& story)
{
    if (npc.submode_66 == 0) {
        return false; // Already initialized; proceed directly to 0x018538 tick
    }

    const uint8_t raw_variant = npc.submode_66;
    const uint8_t v = (raw_variant >= 1 && raw_variant <= 20) ? raw_variant : 1;
    const bool branch_a = story.flag_ff0dbf_bit0 || (!story.flag_ff0dbc_bit0);

    switch (v) {
        case 1:
            npc.height_4a = 0x14;
            npc.anim_2a = 0x06;
            if (branch_a) {
                npc.patrol_box = {0x00E8, 0x01A8, 0x01A8, 0x0210};
                npc.dialog_id_a0 = 0x0001;
            } else {
                npc.patrol_box = {0x0048, 0x01C0, 0x0160, 0x02C0};
                npc.dialog_id_a0 = 0x0055;
            }
            break;

        case 2:
            npc.anim_2a = 0x0A;
            if (branch_a) {
                npc.patrol_box = {0x0040, 0x0188, 0x0098, 0x01A8};
                npc.dialog_id_a0 = 0x0002;
            } else {
                npc.patrol_box = {0x0050, 0x0188, 0x0138, 0x01E0};
                npc.dialog_id_a0 = 0x0056;
            }
            break;

        case 3:
            npc.anim_2a = 0x0E;
            if (branch_a) {
                npc.patrol_box = {0x0070, 0x0288, 0x0120, 0x02E0};
                npc.dialog_id_a0 = 0x0003;
            } else {
                npc.patrol_box = {0x0020, 0x01D8, 0x0178, 0x02E0};
                npc.dialog_id_a0 = 0x0057;
            }
            break;

        case 4:
            npc.anim_2a = 0x12;
            if (branch_a) {
                npc.patrol_box = {0x00C8, 0x01A8, 0x00E8, 0x0240};
                npc.dialog_id_a0 = 0x0004;
            } else {
                npc.patrol_box = {0x00D0, 0x00F8, 0x0170, 0x01F0};
                npc.dialog_id_a0 = 0x0058;
            }
            break;

        case 5:
            npc.height_4a = 0x10;
            npc.flag_3c |= (1 << 2);
            npc.flag_37 |= (1 << 6) | (1 << 7);
            npc.anim_2a = 0x14;
            if (!story.flag_ff0dbf_bit0) {
                npc.dialog_id_a0 = 0x0005;
            } else if (!story.flag_ff0dc5_bit3) {
                npc.dialog_id_a0 = 0x0059;
            } else {
                npc.dialog_id_a0 = 0x005D;
            }
            break;

        case 6:
            npc.height_4a = 0x16;
            npc.flag_37 |= (1 << 6) | (1 << 7);
            npc.anim_2a = 0x16;
            npc.facing_16 = 3;
            npc.dialog_id_a0 = 0x0006;
            break;

        case 7:
            npc.flag_38 |= (1 << 2) | (1 << 0);
            npc.flag_36 |= (1 << 2);
            npc.shadow_3d = 0;
            npc.dialog_id_a0 = 0;
            npc.anim_2a = 0x1E;
            break;

        case 8:
            npc.height_4a = 0x08;
            npc.dialog_id_a0 = 0;
            npc.anim_2a = 0x20;
            if (branch_a) {
                npc.patrol_box = {0x00C8, 0x01D0, 0x01A0, 0x0228};
            } else {
                npc.patrol_box = {0x0020, 0x00C0, 0x0170, 0x0238};
            }
            break;

        case 9:
            npc.anim_2a = 0x28;
            npc.z_10 = 0x0050;
            npc.flag_38 |= (1 << 2);
            npc.flag_36 |= (1 << 3);
            npc.dialog_id_a0 = 0;
            break;

        case 10:
            npc.height_4a = 0x14;
            npc.anim_2a = 0x06;
            if (branch_a) {
                npc.patrol_box = {0x00E0, 0x01D0, 0x0168, 0x02A8};
                npc.dialog_id_a0 = 0x000A;
            } else {
                npc.patrol_box = {0x0010, 0x0088, 0x0130, 0x0168};
                npc.dialog_id_a0 = 0x005B;
            }
            break;

        case 11:
            npc.anim_2a = 0x0E;
            if (branch_a) {
                npc.patrol_box = {0x00D0, 0x0100, 0x0138, 0x01A0};
                npc.dialog_id_a0 = 0x000B;
            } else {
                npc.patrol_box = {0x00D0, 0x0088, 0x01D8, 0x01E0};
                npc.dialog_id_a0 = 0x005C;
            }
            break;

        case 12:
            npc.height_4a = 0x14;
            npc.anim_2a = 0x06;
            npc.patrol_box = {0x00B8, 0x0218, 0x0150, 0x03F0};
            npc.dialog_id_a0 = 0x0029;
            break;

        case 13:
            npc.height_4a = 0x14;
            npc.anim_2a = 0x06;
            npc.patrol_box = {0x0070, 0x0320, 0x0190, 0x03F0};
            npc.dialog_id_a0 = 0x0029;
            break;

        case 14:
            npc.anim_2a = 0x12;
            npc.patrol_box = {0x0038, 0x0208, 0x0118, 0x02C8};
            npc.dialog_id_a0 = 0x002D;
            break;

        case 15:
            npc.anim_2a = 0x0E;
            npc.patrol_box = {0x00D0, 0x02E8, 0x0190, 0x03F0};
            npc.dialog_id_a0 = 0x002C;
            break;

        case 16:
            npc.anim_2a = 0x0A;
            npc.patrol_box = {0x00C8, 0x0218, 0x0138, 0x03C0};
            npc.dialog_id_a0 = 0x002B;
            break;

        case 17:
            npc.anim_2a = 0x0A;
            npc.patrol_box = {0x0118, 0x0210, 0x01B0, 0x0268};
            npc.dialog_id_a0 = 0x002A;
            break;

        case 18:
            npc.anim_2a = 0x0A;
            npc.facing_16 = 2;
            npc.dialog_id_a0 = 0x0031;
            npc.flag_37 |= (1 << 6);
            break;

        case 19:
            npc.anim_2a = 0x0E;
            npc.facing_16 = 2;
            npc.dialog_id_a0 = 0x0032;
            npc.flag_37 |= (1 << 6);
            break;

        case 20:
            npc.height_4a = 0x14;
            npc.anim_2a = 0x06;
            npc.facing_16 = 2;
            npc.dialog_id_a0 = 0x0033;
            npc.flag_37 |= (1 << 6);
            break;
    }

    // Common Epilogue (0x018526..0x018538)
    npc.saved_variant_9e = raw_variant;
    npc.submode_66 = 0;
    npc.timer_2 = 0;
    return true;
}

} // namespace Thor::TownNPC
```
