# STANDALONE HANDOFF: M15.432 — ROM Room Warp Controller, Camera Centering, Cutscene Locks & Player Ali Spawn Reset Engine

## 1. Routine Overview
- **Addresses:** `0x00C9B2..0x00CC12` (608 bytes)
- **Primary Entrypoints:**
  - `0x00C9B2`: `check_cutscene_script_lock` (Evaluates cutscene, dialogue, and transition lock flags; aborts caller on lock)
  - `0x00C9EC`: `warp_room_entry_immediate` (Sets up immediate warp, centers camera on spawn position)
  - `0x00CA10`: `warp_room_entry_fade` (Sets up blackout/fade-out warp, centers camera on spawn position)
  - `0x00CA3E`: `room_transition_execute` (Room ID remapping, sound engine sync, BGM loader, entity reset)
  - `0x00CB64`: `spawn_player_ali_in_room` (Resets Player Ali slot at `$FF19E8`, applies sky drop pose if fade active)

## 2. Cutscene & Transition Locks (`0x00C9B2..0x00C9EA`)
Checks 6 flags in RAM:
- `$FF17B9 != 0`: Dialogue or cutscene active
- `$FF1987 != 0`: Transition busy
- `$FF1985 != 0`: Pause / menu active
- `$FF17B8 & 0x04 != 0`: Cutscene lock bit 2
- `$FF1983 != 0`, `$FF1982 != 0`: Secondary transition locks
If any is set, pops the return address off the stack (`addq.l #4, a7; rts`) to abort the initiating action.

## 3. Camera Centering Math (`0x00C9F8..0x00CA0C`, `0x00CA24..0x00CA38`)
Given spawn coordinates `(target_x, target_y)`:
- `camera_x = (target_x - 160) & 0xFFFF` (centers 320-pixel Genesis display)
- `camera_y = (target_y - 112) & 0xFFFF` (centers 224-pixel Genesis display)
- Packed camera register: `((camera_x << 16) | camera_y)`
- Packed spawn register: `((target_x << 16) | target_y)`

## 4. Room Overrides & Script Header (`0x00CA3E..0x00CA64`)
- Special override: if `room_id == 4`, replaces `room_id = 0x0611`.
- Calls script pointer resolver (`0x00C90E`).
- Reads bit 6 on script header byte `+0x19`: if set, invokes BGM music engine (`$17DA`).
- Calls sound driver (`$60004`, `d0 = 2`) to mute SFX/voice channels.

## 5. Player Ali Spawn Slot (`$FF19E8`, `0x00CB64..0x00CC12`)
- Sets coordinates: `$8(a0) = spawn_x`, `$C(a0) = spawn_y`.
- Clears elevation `$10(a0) = 0`, physics velocities/timers (`$72, $76, $7A`).
- Sets default status:
  - Mode `$4(a0) = 0`, Facing `$2A(a0) = 0`
  - Invalidation cache tags: `$2C = 0xFFFF`, `$34 = 0xFFFF`, `$5C = 0xFFFF`
  - Idle animation frame: `$32 = 0x003F`
  - Visibility flag: `$36 = 0x01`
  - Tangible layer: `$38 = 0x10`
- **Sky Drop Falling Pose:**
  - If fade flag `$FF17BC != 0` or `$FF17BB != 0`:
    - Sets falling elevation: `$10(a0) = 0x00F0` (240 pixels in air!)
    - Sets action mode and facing: `$4(a0) = 6`, `$2A(a0) = 6` (falling drop pose!)
    - Sets falling animation counter: `$56(a0) = 1`

## 6. Cleanroom C++20 Interface
```cpp
namespace thor::warp {

struct CameraCoords {
    int16_t cam_x{0};
    int16_t cam_y{0};
    int16_t spawn_x{0};
    int16_t spawn_y{0};
};

struct PlayerSpawnState {
    int16_t x{0};
    int16_t y{0};
    int16_t elevation{0};
    uint16_t mode{0};
    uint16_t facing{0};
    uint16_t idle_frame{0x003F};
    bool visible{true};
    uint8_t layer{0x10};
};

CameraCoords calculate_camera_center(int16_t target_x, int16_t target_y);
uint16_t apply_room_id_overrides(uint16_t room_id);
PlayerSpawnState spawn_player_ali(const CameraCoords& coords, bool fade_transition);

} // namespace thor::warp
```
