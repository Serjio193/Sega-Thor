#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace oasis::game::world {

/// Presentation mode determined by $FF17BF and $FF17BE (0x00CD64..0x00CE22).
enum class WarpPresentationMode : std::uint8_t {
    StandardFade = 0U,
    WhiteoutFlash = 1U,
    SmoothCrossfade = 2U,
    InstantFootsteps = 3U,
};

/// Footstep audio jingle step record (0x00CE72).
struct FootstepJingleStep {
    std::uint32_t sound_id{0U};
    std::uint8_t delay_frames{14U};
};

/// High-level presentation configuration for room entry transition.
struct RoomPresentationConfig {
    WarpPresentationMode mode{WarpPresentationMode::StandardFade};
    bool room_flag_bit1{false};       // ($FF16F1 & 0x02) != 0
    bool trigger_exit_sfx{false};     // $FF17BB != 0
    bool suppress_exit_sfx{false};    // $FF17BC != 0
    std::uint32_t saved_sp{0x00FFFE00U};
};

/// Result of executing room entry presentation sequence.
struct RoomPresentationResult {
    bool whiteout_applied{false};
    std::uint8_t crossfade_steps_completed{0U};
    bool standard_fade_called{false};
    bool instant_commit_called{false};
    bool footsteps_played{false};
    bool room_flag_164f_set{false};
    bool sfx_suppress_cleared{false};
    bool jumped_to_gameplay{false};
    std::uint32_t restored_sp{0U};
    std::vector<std::uint32_t> sound_commands_emitted;
    std::vector<std::uint8_t> frame_delays;
};

/// Configuration and constants for ROM Room Entry Presentation Manager,
/// Palette Crossfader & Footstep Audio Jingle (0x00CD64..0x00CEEA, 390 bytes).
struct RoomPresentationJingleConfig {
    static constexpr std::uint32_t kEntryPresentationManagerAddr = 0x00CD64U;
    static constexpr std::uint32_t kEntryInstantWarpStepsAddr = 0x00CD7EU;
    static constexpr std::uint32_t kEntryPaletteCrossfadeAddr = 0x00CDA8U;
    static constexpr std::uint32_t kEntryWhiteoutFlashAddr = 0x00CDECU;
    static constexpr std::uint32_t kEntryStandardFadeAddr = 0x00CE22U;
    static constexpr std::uint32_t kEntryPostFadeFinalizeAddr = 0x00CE2EU;
    static constexpr std::uint32_t kEntryFootstepsJingleAddr = 0x00CE72U;
    static constexpr std::uint32_t kEntryCramBufferCopyAddr = 0x00CECCU;
    static constexpr std::uint32_t kRangeEndAddr = 0x00CEEAU;
    static constexpr std::size_t kRoutineSizeBytes = 390U;

    // ROM Addresses & Constants
    static constexpr std::uint32_t kTargetWhitePaletteAddr = 0x05D686U;
    static constexpr std::size_t kWhitePaletteWordCount = 64U;
    static constexpr std::uint32_t kGameplayLoopDestAddr = 0x008AA6U;

    // RAM Addresses
    static constexpr std::uint32_t kCramBufferBaseAddr = 0x00FF134CU;
    static constexpr std::uint32_t kCramPaletteLine1Addr = 0x00FF138CU;
    static constexpr std::uint32_t kCramDirtyFlagsAddr = 0x00FF164DU;
    static constexpr std::uint32_t kDmaFadeBusyAddr = 0x00FF164EU;
    static constexpr std::uint32_t kRoomFlagOutAddr = 0x00FF164FU;
    static constexpr std::uint32_t kRoomCustomFlagAddr = 0x00FF16F1U;
    static constexpr std::uint32_t kSourcePalettePtrAddr = 0x00FF179EU;
    static constexpr std::uint32_t kWarpSfxTriggerAddr = 0x00FF17BBU;
    static constexpr std::uint32_t kWarpSfxSuppressAddr = 0x00FF17BCU;
    static constexpr std::uint32_t kWarpModeTypeAddr = 0x00FF17BEU;
    static constexpr std::uint32_t kSpecialFadeModeAddr = 0x00FF17BFU;
    static constexpr std::uint32_t kFadeTimerAddr = 0x00FF1896U;
    static constexpr std::uint32_t kPaletteScratchBufferAddr = 0x00FF18A6U;
    static constexpr std::uint32_t kSavedStackPointerAddr = 0x00FF1930U;
    static constexpr std::uint32_t kSoundDriverPort = 0x00060004U;

    // Sound Command IDs
    static constexpr std::uint32_t kSndFootstepA = 0x0090038EU;
    static constexpr std::uint32_t kSndFootstepB = 0x0090038FU;
    static constexpr std::uint8_t kFootstepDelayFrames = 14U;
    static constexpr std::uint32_t kSndWarpExitChime = 0x00800230U;

    // Palette & Timing Constants
    static constexpr std::uint16_t kPureWhiteColor = 0x0EEEU;
    static constexpr std::size_t kCrossfadeStepCount = 33U; // 0..32
    static constexpr std::uint8_t kInstantWarpTimer = 4U;
};

/// Emits the 4-step footstep audio jingle sequence (0x00CE72..0x00CECC).
void play_footsteps_jingle(
    std::vector<std::uint32_t>& sound_sink,
    std::vector<std::uint8_t>& delay_sink);

/// Copies 32 words from source palette into destination CRAM Line 1 buffer (0x00CECC..0x00CEEA).
void copy_cram_palette_line1(
    std::span<const std::uint16_t, 32> source_palette,
    std::span<std::uint16_t, 32> dest_cram_line) noexcept;

/// Interpolates a single 9-bit Genesis CRAM color towards target color (0x00302E).
[[nodiscard]] std::uint16_t interpolate_genesis_color(
    std::uint16_t src,
    std::uint16_t dst,
    std::uint8_t step,
    std::uint8_t max_steps = 32U) noexcept;

/// Executes the full room entry presentation manager sequence (0x00CD64..0x00CE72).
[[nodiscard]] RoomPresentationResult execute_room_presentation(
    const RoomPresentationConfig& config,
    std::span<std::uint16_t, 64> cram_palette,
    std::span<std::uint16_t, 64> scratch_buffer,
    std::span<const std::uint16_t, 64> target_white_palette) noexcept;

// Raw 64KB RAM window integration helpers
void execute_whiteout_flash_raw(std::span<std::uint8_t> ram) noexcept;
std::uint8_t execute_palette_crossfade_raw(std::span<std::uint8_t> ram, std::span<const std::uint8_t> rom) noexcept;
void copy_cram_palette_line1_raw(std::span<std::uint8_t> ram) noexcept;
RoomPresentationResult execute_room_presentation_manager_raw(std::span<std::uint8_t> ram, std::span<const std::uint8_t> rom) noexcept;

} // namespace oasis::game::world

namespace thor::environment {
using WarpPresentationMode = oasis::game::world::WarpPresentationMode;
using FootstepJingleStep = oasis::game::world::FootstepJingleStep;
using RoomPresentationConfig = oasis::game::world::RoomPresentationConfig;
using RoomPresentationResult = oasis::game::world::RoomPresentationResult;
using RoomPresentationJingleConfig = oasis::game::world::RoomPresentationJingleConfig;

using oasis::game::world::play_footsteps_jingle;
using oasis::game::world::copy_cram_palette_line1;
using oasis::game::world::interpolate_genesis_color;
using oasis::game::world::execute_room_presentation;
using oasis::game::world::execute_whiteout_flash_raw;
using oasis::game::world::execute_palette_crossfade_raw;
using oasis::game::world::copy_cram_palette_line1_raw;
using oasis::game::world::execute_room_presentation_manager_raw;
} // namespace thor::environment

namespace thor::world {
using namespace thor::environment;
} // namespace thor::world
