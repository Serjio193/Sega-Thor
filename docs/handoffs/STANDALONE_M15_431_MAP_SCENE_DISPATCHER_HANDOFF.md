# STANDALONE HANDOFF: M15.431 — ROM Map & Scene Script Dispatcher, 21-Area Script Table & State Manager

## 1. Routine Overview
- **Addresses:** `0x00C88C..0x00C9B2` (294 bytes)
- **Primary Entrypoints:**
  - `0x00C8AA`: `load_map_scene_script` (Loads scene script, clears area/map state flags on transitions, updates `$FF0E40`, executes init at `+0x1A`)
  - `0x00C90E`: `get_map_scene_script_pointer` (Resolves script pointer in ROM for packed ID without side effects)
  - `0x00C980`: `exec_map_script_tick` (Checks pause flag bit 1 of `$FF17B8`, jumps to tick callback at `+0x00`)
  - `0x00C998`: `exec_map_script_subtick` (Checks pause flag, jumps to subtick callback at `+0x02`)
  - `0x00C88C`: `dialogue_update_caller_anim` (Calls caller animation helper if `$FF1882 != 0`)

## 2. Area Script Pointer Table (`0x00C92C`)
Contains 21 32-bit absolute ROM addresses indexing the relative scene offset tables for every map in the game:
- Area 00: `0x0002C6B8`
- Area 01: `0x0002E56A`
- Area 02: `0x0002F1A4`
- Area 03: `0x0002F9B2`
- Area 04: `0x0002FF80`
- Area 05: `0x00030218`
- Area 06: `0x00030E56`
- Area 07: `0x00031B50`
- Area 08: `0x000328D6`
- Area 09: `0x0003361C`
- Area 0A: `0x000341A4`
- Area 0B: `0x00035528`
- Area 0C: `0x00036286`
- Area 0D: `0x00037B42`
- Area 0E: `0x00038AAC`
- Area 0F: `0x0002C6F2`
- Area 10: `0x00038FBA`
- Area 11: `0x0003957C`
- Area 12: `0x00039F7A`
- Area 13: `0x00039F7C`
- Area 14: `0x00039F7E`

## 3. Scene Script Resolution & Memory Lifecycle (`0x00C8AA..0x00C90C`)
- Input: `d0.w` = packed `(area_id << 8) | scene_id`.
- Transition checks against `$FF0E40` (current packed ID):
  - If `d0 != current_id`:
    - Clears 16 bytes at `$FF0E2C..$FF0E3C` (area state flags).
    - If `(d0 >> 8) != (current_id >> 8)` (area changed):
      - Clears 16 bytes at `$FF0E1C..$FF0E2C` (map state flags).
      - Clears 3 bytes at `$FF0DB3..$FF0DB6` (area trigger flags).
    - Updates `$FF0E40 = d0`.
- Script pointer calculation:
  ```cpp
  uint32_t area_base = area_table[area_id];
  int16_t scene_offset = read_be16(rom.data() + area_base + scene_id * 2);
  uint32_t script_ptr = (area_base + scene_id * 2) + scene_offset;
  ```
- Saves `script_ptr` into `$FF16EC`.
- Initialization routine entry: `script_ptr + 0x1A`.

## 4. Script Execution Tick & Subtick (`0x00C980`, `0x00C998`)
- Both checks test bit 1 of `$FF17B8`: if bit 1 is set (`btst.b #1, $FF17B8.l; bne return`), script ticks are paused / bypassed.
- Tick (`0x00C980`):
  `target = script_ptr + read_be16(rom.data() + script_ptr)`
- Subtick (`0x00C998`):
  `target = (script_ptr + 2) + read_be16(rom.data() + script_ptr + 2)`

## 5. Cleanroom C++20 Interface
```cpp
namespace thor::script {

struct MapState {
    uint16_t current_id{0};
    std::array<uint8_t, 16> area_flags{};
    std::array<uint8_t, 16> map_flags{};
    std::array<uint8_t, 3> trigger_flags{};
    uint32_t active_script_ptr{0};
    bool script_paused{false};
};

uint32_t resolve_scene_script_pointer(std::span<const uint8_t> rom, uint16_t packed_id);
uint32_t load_scene_script(MapState& state, std::span<const uint8_t> rom, uint16_t target_id);
bool can_tick_script(const MapState& state);
uint32_t get_script_tick_target(std::span<const uint8_t> rom, uint32_t script_ptr);
uint32_t get_script_subtick_target(std::span<const uint8_t> rom, uint32_t script_ptr);

} // namespace thor::script
```
