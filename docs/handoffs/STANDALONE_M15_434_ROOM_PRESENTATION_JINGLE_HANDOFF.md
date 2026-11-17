# STANDALONE HANDOFF: M15.434 — ROM Room Entry Presentation Manager, Palette Crossfader & Footstep Audio Jingle

## 1. Routine Overview
- **Addresses:** `0x00CD64..0x00CEEA` (390 bytes)
- **Primary Entrypoints:**
  - `0x00CD64`: `room_entry_presentation_manager` (Orchestrates room entry fade/crossfade, whiteout flashes, instant footstep transitions, and hands off execution to the gameplay loop `$8AA6`)
  - `0x00CDEC`: `room_entry_whiteout_flash` (Fills 64 CRAM words with pure white `#$0EEE`, sets DMA flags `$FF164D` bits 0 & 1, waits for DMA acknowledge)
  - `0x00CDA8`: `room_entry_palette_crossfade` (Copies 64 CRAM words to scratch `$FF18A6`, executes 33-step RGB crossfade loop to target base `$05D686` via `$302E`)
  - `0x00CE22`: `room_entry_standard_fade` (Clears fade timer `$FF1896`, triggers standard palette fade-in `$2D84`)
  - `0x00CD7E`: `room_entry_instant_warp_steps` (Sets timer `$FF1896 = 4`, commits palette `$2DB6`, plays 4-step footstep audio jingle `$CE72`, waits for hardware bit 4)
  - `0x00CE72`: `room_entry_footsteps_jingle` (Emits 4 alternating footstep sounds `$0090038E` and `$0090038F` with 14-frame pacing delays)
  - `0x00CECC`: `cram_palette_buffer_copy` (Copies 32 words from source palette pointer `$FF179E` into CRAM Line 1 buffer `$FF138C`)

## 2. Room Presentation Mode Dispatch (`0x00CD64..0x00CE2E`)
```
                     tst.b $FF164E (wait if busy)
                                |
                     tst.b $FF17BF (special mode?)
                       /         \
                 [!= 0]           [== 0]
                   /                 \
        cmpi.b #1, $FF17BF      tst.b $FF17BE (warp type?)
          /          \               /          \
     [== 1]         [!= 1]        [!= 0]       [== 0]
       |              |             |            |
   Whiteout       33-Step        Instant      Standard
    Flash        Crossfade      Footsteps     Fade-In
  (0x00CDEC)    (0x00CDA8)     (0x00CD7E)   (0x00CE22)
       \              /             |            /
        \____________/              |___________/
                    \                /
                     Post-Fade Setup (0x00CE2E)
```

## 3. Whiteout Flash & Palette Crossfade Logic
- **Whiteout Flash (`0x00CDEC`):**
  - Clears mode flag: `$FF17BF = 0`.
  - Fills 32 longwords (64 words, 128 bytes) at `$FF134C` with `#$0EEE0EEE`.
  - Sets CRAM dirty flags: `$FF164D |= 0x03` (bits 0 and 1).
  - Spin-waits until V-INT CRAM DMA finishes and clears bit 1: `btst #1, $FF164D; bne ...`.
- **Palette Crossfade (`0x00CDA8`):**
  - Clears mode flag: `$FF17BF = 0`.
  - Copies 64 words from `$FF134C` to scratch buffer `$FF18A6`.
  - Iterates 33 steps (`d2 = 0x20` down to 0):
    - `d0 = step` (0..32)
    - `d1 = 0x3F` (64 colors)
    - `a0 = $FF134C` (destination buffer)
    - `a1 = $05D686` (ROM target pure white palette)
    - `a2 = $FF18A6` (source buffer)
    - Calls palette interpolation routine `jsr $302E`.

## 4. Footstep Audio Jingle Sequence (`0x00CE72..0x00CECC`)
- Preserves `d0` on stack.
- Step 1: Send sound command `0x0090038E` to `$60004`, delay 14 frames via `jsr $32E8`.
- Step 2: Send sound command `0x0090038F` to `$60004`, delay 14 frames via `jsr $32E8`.
- Step 3: Send sound command `0x0090038E` to `$60004`, delay 14 frames via `jsr $32E8`.
- Step 4: Send sound command `0x0090038F` to `$60004`, delay 14 frames via `jsr $32E8`.
- Restores `d0` and returns (`rts`).

## 5. Post-Fade Finalization & Gameplay Handoff (`0x00CE2E..0x00CE72`)
- Room custom flag check: if bit 1 of `$FF16F1` is set, sets `$FF164F = 0xFF`.
- Warp exit SFX:
  - If `$FF17BB != 0` and `$FF17BC == 0`, sends sound command `0x00800230` to sound driver `$60004`.
- SFX suppress reset: if `$FF17BC != 0`, clears `$FF17BC = 0`.
- Stack pointer recovery: restores stack pointer from saved pointer at `$FF1930` (`movea.l $FF1930, a7`).
- Gameplay loop jump: branches directly to core game loop `bra.w $8AA6`.

## 6. Cleanroom C++20 Interface
```cpp
namespace thor::environment {

enum class WarpPresentationMode : uint8_t {
    StandardFade = 0,
    WhiteoutFlash = 1,
    SmoothCrossfade = 2,
};

struct RoomPresentationConfig {
    WarpPresentationMode mode{WarpPresentationMode::StandardFade};
    bool instant_footsteps{false};
    bool room_flag_bit1{false};
    bool trigger_exit_sfx{false};
    bool suppress_exit_sfx{false};
};

struct RoomPresentationResult {
    bool whiteout_applied{false};
    uint8_t crossfade_steps_completed{0};
    bool footsteps_played{false};
    bool room_flag_164f_set{false};
    std::vector<uint32_t> sound_commands_emitted;
};

RoomPresentationResult execute_room_presentation(
    const RoomPresentationConfig& config,
    std::span<uint16_t, 64> cram_palette,
    std::span<const uint16_t, 64> target_white_palette);

void play_footsteps_jingle(std::vector<uint32_t>& sound_sink);

void copy_cram_palette_line(
    std::span<const uint16_t, 32> source_palette,
    std::span<uint16_t, 32> dest_cram_line);

} // namespace thor::environment
```
