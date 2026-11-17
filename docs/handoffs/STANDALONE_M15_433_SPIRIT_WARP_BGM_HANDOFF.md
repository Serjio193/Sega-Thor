# STANDALONE HANDOFF: M15.433 — ROM Companion Spirit Warp Repositioning, Room Collision Table Loader & BGM Engine Synchronizer

## 1. Routine Overview
- **Addresses:** `0x00CC12..0x00CD64` (338 bytes)
- **Primary Entrypoints:**
  - `0x00CC12`: `reposition_spirit_on_room_warp` (Synchronizes companion spirit `$FF1AA4` position and state to Player Ali `$FF19E8` upon room entry)
  - `0x00CCA4`: `load_room_collision_table` (Loads 32-area room collision pointer from ROM `0x04371E`, sets collision tick cadence)
  - `0x00CCE6`: `init_room_environment_and_sync_bgm` (Checks spirit forbid flag `$FF16F1` bit 0, executes first room tick, syncs BGM track with sound driver `$60004`)
  - `0x00CD4A`: `latch_all_entity_ground_surfaces` (Sweeps all 21 Group 2 entities, invoking `0x009D00` to latch ground elevation and surface attributes)

## 2. Companion Spirit Repositioning (`0x00CC12..0x00CCA4`)
- Operates on active spirit entity slot `$FF1AA4` relative to Player Ali `$FF19E8`:
  - Clears mode byte: `+0x39(a1) = 0`.
  - Type-specific handling:
    - Type `0x0016` (Bow): unsummoned (`+0x00(a1) = 0`). Bow cannot carry across room warps.
    - Type `0x001C` (Dytto / Efreet): calls state synchronization helper `$1378A`.
    - Type `0x001E` (Shade): calls projection helper `$147E0` with Ali coords `(X, Y, elevation)`.
    - Type `0x001A`: sets position to `(ali_x, ali_y - 8)`, clears mode `+0x04 = 0`, facing `+0x2A = 0`, state counters `+0x9E = 0`, `+0xA2 = 0`, subpixel vel `+0x06 = 0`.
    - Other types: sets position to `(ali_x, ali_y - 8)`.
- Room Spirit Forbid Flag: if `$FF16F1` bit 0 is set, unsummons spirit unconditionally (`+0x00(a1) = 0`).

## 3. Collision Table Pointer Resolution (`0x00CCA4..0x00CCD4`)
- Loads from ROM table `0x04371E` containing 32 area pointers.
- Calculation:
  ```cpp
  uint32_t area_base = collision_area_table[area_id];
  int16_t scene_offset = read_be16(rom.data() + area_base + scene_id * 2);
  uint32_t collision_ptr = (area_base + scene_id * 2) + scene_offset;
  ```
- Saves `collision_ptr` into `$FF17B2`.
- Sets collision timer cadence `$FF17B7 = 10` (`0x0A` frames).

## 4. BGM Music Engine Synchronization (`0x00CD2C..0x00CD44`)
- Compares target track `$FF16F4` with current playing track `$FF16F2`:
  - If `target == current`: sends sound driver command `3` (continue track without restarting playback) to `$60004`.
  - If `target != current`: updates `$FF16F2 = target`, sends track command `target` to `$60004`.

## 5. Group 2 Entity Ground Latch Sweep (`0x00CD4A..0x00CD64`)
- Iterates 21 slots at `$FF19E8` (stride 188 bytes / `0xBC`):
  - If slot active (`type > 0`): invokes ground latch `0x009D00` (`latch_entity_ground_surface`).

## 6. Cleanroom C++20 Interface
```cpp
namespace thor::environment {

struct SpiritState {
    bool active{false};
    uint16_t type{0};
    int16_t x{0};
    int16_t y{0};
    uint16_t mode{0};
    uint16_t facing{0};
};

struct BgmSyncResult {
    uint8_t sound_cmd{0};
    uint8_t current_bgm{0};
};

void reposition_spirit_on_warp(SpiritState& spirit, int16_t ali_x, int16_t ali_y, bool room_forbids_spirits);
uint32_t resolve_collision_table_pointer(std::span<const uint8_t> rom, uint16_t packed_id);
BgmSyncResult synchronize_bgm_track(uint8_t current_bgm, uint8_t target_bgm);

} // namespace thor::environment
```
