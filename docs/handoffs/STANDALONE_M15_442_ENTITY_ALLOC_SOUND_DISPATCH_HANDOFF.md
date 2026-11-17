# Standalone Handoff: M15.442 Projectile/Effect Entity Pool Allocators & Sound Driver Command Dispatchers

## Routine Overview
- **Source ROM:** `local-roms/Beyond Oasis (USA).md`
- **ROM Address Range:** `0x00D9A4..0x00DB1A` (374 bytes, 96 instructions)
- **Primary Subroutines:**
  - `0x00D9A4`: `decompress_palette_block` (calls `0x0037D2` into `$FF2FA8`, returns via `rts` at `0x00D9BC`).
  - `0x00D9BE`: `alloc_projectile_entity_slot` (scans 8 slots at `$FF2ABC` with stride `0x5A`, tile index `#$0694 + 9*slot`, returns via `rts` at `0x00D9EE`).
  - `0x00D9F0`: `alloc_spark_effect_slot` (scans 6 slots at `$FF2D8C` with stride `0x5A`, returns via `rts` at `0x00DA18`).
  - `0x00DA1A`: `find_free_general_entity_slot_18` (scans 18 slots at `$FF1C1C` with stride `0xBC`, returns via `rts` at `0x00DA60`).
  - `0x00DA2A`: `find_free_general_entity_slot_17` (scans 17 slots at `$FF1CD8` with stride `0xBC`, returns via `rts` at `0x00DA60`).
  - `0x00DA62`: `alloc_spawn_npc_entity_slot_18` (scans 18 slots, sets type `#$002E`, calls `0x008D06`, sets timer `#$003C`, returns via `rts` at `0x00DAC0`).
  - `0x00DA72`: `alloc_spawn_npc_entity_slot_17` (scans 17 slots, sets type `#$002E`, calls `0x008D06`, sets timer `#$003C`, returns via `rts` at `0x00DAC0`).
  - `0x00DAC2`: `sound_play_music_track` (saves track ID to `$FF16F2`, jumps to `$60004`).
  - `0x00DACE`: `sound_resume_music_track` (checks `$FF16F2`, calls `$60004` if not negative, returns via `rts` at `0x00DAE4`).
  - `0x00DAE6`: `sound_pause_music_track` (sets `$FF16F2 = -1`, calls `$60004` with cmd 7, returns via `rts` at `0x00DAFE`).
  - `0x00DB00`: `sound_stop_music_track` (sets `$FF16F2 = -1`, calls `$60004` with cmd 0, returns via `rts` at `0x00DB18`).

---

## Memory & Entity Pool Layout

| Pool / Register | Base Address | Slot Count | Slot Stride | Return Value / Behavior |
|-----------------|--------------|------------|-------------|-------------------------|
| Projectile Pool | `$FF2ABC`    | 8 slots    | 90 bytes (`0x5A`) | `a6`: slot ptr, `d0`: tile index (`0x0694 + 9*i`), CCR carry: clear=OK, set=Full |
| Spark Effect Pool | `$FF2D8C`  | 6 slots    | 90 bytes (`0x5A`) | `a6`: slot ptr, CCR carry: clear=OK, set=Full |
| General Pool (18) | `$FF1C1C`  | 18 slots   | 188 bytes (`0xBC`) | `a6`: slot ptr, clears `$FF184F` if `$FF1850 == a6`, CCR carry: clear=OK, set=Full |
| General Pool (17) | `$FF1CD8`  | 17 slots   | 188 bytes (`0xBC`) | `a6`: slot ptr, clears `$FF184F` if `$FF1850 == a6`, CCR carry: clear=OK, set=Full |
| Spawn NPC Pool (18/17) | `$FF1C1C` / `$FF1CD8` | 18 / 17 | 188 bytes | Initializes slot `+0x00 = 0x002E`, calls `0x008D06`, sets `+0x02 = 0x003C`, `d0 = a6 - $FF19E8` |
| `$FF16F2`       | `int16_t`    | Active track ID | - | Stored by `sound_play_music_track`, checked by resume, set to `-1` on pause/stop |
| `$060004`       | Sound Driver | Trap / Entry | - | Sega sound driver entrypoint |

---

## C++20 Interface Proposal

```cpp
#pragma once

#include <cstdint>
#include <optional>

namespace thor::entities {

struct ProjectileSlotAllocation {
    uint32_t slot_ram_addr{0};
    uint16_t vram_tile_index{0};
    uint8_t slot_index{0};
};

class EntityPoolManager {
public:
    static constexpr uint32_t kProjectilePoolBase = 0xFF2ABC;
    static constexpr uint8_t  kProjectileSlots = 8;
    static constexpr uint32_t kProjectileStride = 0x5A;
    static constexpr uint16_t kProjectileBaseTile = 0x0694;
    static constexpr uint16_t kProjectileTileStep = 9;

    static constexpr uint32_t kSparkPoolBase = 0xFF2D8C;
    static constexpr uint8_t  kSparkSlots = 6;
    static constexpr uint32_t kSparkStride = 0x5A;

    static constexpr uint32_t kGeneralPool18Base = 0xFF1C1C;
    static constexpr uint8_t  kGeneralPool18Slots = 18;
    static constexpr uint32_t kGeneralPool17Base = 0xFF1CD8;
    static constexpr uint8_t  kGeneralPool17Slots = 17;
    static constexpr uint32_t kGeneralPoolStride = 0xBC;

    static constexpr uint32_t kPlayerEntityBase = 0xFF19E8;

    static std::optional<ProjectileSlotAllocation> AllocProjectileSlot(const uint16_t* slot_status_array) {
        for (uint8_t i = 0; i < kProjectileSlots; ++i) {
            if (slot_status_array[i] == 0) {
                return ProjectileSlotAllocation{
                    kProjectilePoolBase + i * kProjectileStride,
                    static_cast<uint16_t>(kProjectileBaseTile + i * kProjectileTileStep),
                    i
                };
            }
        }
        return std::nullopt;
    }
};

} // namespace thor::entities
```
