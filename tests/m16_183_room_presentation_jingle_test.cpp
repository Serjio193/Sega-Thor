#include "game/world/room_presentation_jingle.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace oasis::game::world;

namespace {

void write_be16_vec(std::vector<std::uint8_t>& buf, std::size_t offset, std::uint16_t val) {
    if (offset + 1U < buf.size()) {
        buf[offset] = static_cast<std::uint8_t>((val >> 8) & 0xFFU);
        buf[offset + 1U] = static_cast<std::uint8_t>(val & 0xFFU);
    }
}

void write_be32_vec(std::vector<std::uint8_t>& buf, std::size_t offset, std::uint32_t val) {
    if (offset + 3U < buf.size()) {
        buf[offset] = static_cast<std::uint8_t>((val >> 24) & 0xFFU);
        buf[offset + 1U] = static_cast<std::uint8_t>((val >> 16) & 0xFFU);
        buf[offset + 2U] = static_cast<std::uint8_t>((val >> 8) & 0xFFU);
        buf[offset + 3U] = static_cast<std::uint8_t>(val & 0xFFU);
    }
}

std::uint16_t read_be16(const std::vector<std::uint8_t>& buf, std::size_t offset) {
    if (offset + 1U >= buf.size()) {
        return 0U;
    }
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(buf[offset]) << 8) |
                                      static_cast<std::uint16_t>(buf[offset + 1U]));
}

} // namespace

static void test_rom_constants_and_layout() {
    assert(RoomPresentationJingleConfig::kEntryPresentationManagerAddr == 0x00CD64U);
    assert(RoomPresentationJingleConfig::kEntryInstantWarpStepsAddr == 0x00CD7EU);
    assert(RoomPresentationJingleConfig::kEntryPaletteCrossfadeAddr == 0x00CDA8U);
    assert(RoomPresentationJingleConfig::kEntryWhiteoutFlashAddr == 0x00CDECU);
    assert(RoomPresentationJingleConfig::kEntryStandardFadeAddr == 0x00CE22U);
    assert(RoomPresentationJingleConfig::kEntryPostFadeFinalizeAddr == 0x00CE2EU);
    assert(RoomPresentationJingleConfig::kEntryFootstepsJingleAddr == 0x00CE72U);
    assert(RoomPresentationJingleConfig::kEntryCramBufferCopyAddr == 0x00CECCU);
    assert(RoomPresentationJingleConfig::kRangeEndAddr == 0x00CEEAU);
    assert(RoomPresentationJingleConfig::kRoutineSizeBytes == 390U);

    assert(RoomPresentationJingleConfig::kTargetWhitePaletteAddr == 0x05D686U);
    assert(RoomPresentationJingleConfig::kWhitePaletteWordCount == 64U);
    assert(RoomPresentationJingleConfig::kGameplayLoopDestAddr == 0x008AA6U);

    assert(RoomPresentationJingleConfig::kCramBufferBaseAddr == 0x00FF134CU);
    assert(RoomPresentationJingleConfig::kCramPaletteLine1Addr == 0x00FF138CU);
    assert(RoomPresentationJingleConfig::kCramDirtyFlagsAddr == 0x00FF164DU);
    assert(RoomPresentationJingleConfig::kDmaFadeBusyAddr == 0x00FF164EU);
    assert(RoomPresentationJingleConfig::kRoomFlagOutAddr == 0x00FF164FU);
    assert(RoomPresentationJingleConfig::kRoomCustomFlagAddr == 0x00FF16F1U);
    assert(RoomPresentationJingleConfig::kSourcePalettePtrAddr == 0x00FF179EU);
    assert(RoomPresentationJingleConfig::kWarpSfxTriggerAddr == 0x00FF17BBU);
    assert(RoomPresentationJingleConfig::kWarpSfxSuppressAddr == 0x00FF17BCU);
    assert(RoomPresentationJingleConfig::kWarpModeTypeAddr == 0x00FF17BEU);
    assert(RoomPresentationJingleConfig::kSpecialFadeModeAddr == 0x00FF17BFU);
    assert(RoomPresentationJingleConfig::kFadeTimerAddr == 0x00FF1896U);
    assert(RoomPresentationJingleConfig::kPaletteScratchBufferAddr == 0x00FF18A6U);
    assert(RoomPresentationJingleConfig::kSavedStackPointerAddr == 0x00FF1930U);
    assert(RoomPresentationJingleConfig::kSoundDriverPort == 0x00060004U);

    assert(RoomPresentationJingleConfig::kSndFootstepA == 0x0090038EU);
    assert(RoomPresentationJingleConfig::kSndFootstepB == 0x0090038FU);
    assert(RoomPresentationJingleConfig::kFootstepDelayFrames == 14U);
    assert(RoomPresentationJingleConfig::kSndWarpExitChime == 0x00800230U);

    assert(RoomPresentationJingleConfig::kPureWhiteColor == 0x0EEEU);
    assert(RoomPresentationJingleConfig::kCrossfadeStepCount == 33U);
    assert(RoomPresentationJingleConfig::kInstantWarpTimer == 4U);

    std::cout << "[PASS] test_rom_constants_and_layout\n";
}

static void test_whiteout_flash_mode() {
    RoomPresentationConfig config;
    config.mode = WarpPresentationMode::WhiteoutFlash;
    config.saved_sp = 0x00FFFE80U;

    std::array<std::uint16_t, 64> cram{};
    cram.fill(0x0222U);
    std::array<std::uint16_t, 64> scratch{};
    std::array<std::uint16_t, 64> target_white{};
    target_white.fill(0x0EEEU);

    const auto res = execute_room_presentation(config, cram, scratch, target_white);

    assert(res.whiteout_applied);
    for (const auto c : cram) {
        assert(c == 0x0EEEU);
    }
    assert(res.jumped_to_gameplay);
    assert(res.restored_sp == 0x00FFFE80U);

    std::cout << "[PASS] test_whiteout_flash_mode\n";
}

static void test_smooth_crossfade_mode() {
    RoomPresentationConfig config;
    config.mode = WarpPresentationMode::SmoothCrossfade;
    config.saved_sp = 0x00FFFE00U;

    std::array<std::uint16_t, 64> cram{};
    cram.fill(0x0000U); // Black initial palette
    std::array<std::uint16_t, 64> scratch{};
    std::array<std::uint16_t, 64> target_white{};
    target_white.fill(0x0EEEU);

    const auto res = execute_room_presentation(config, cram, scratch, target_white);

    assert(res.crossfade_steps_completed == 33U);
    for (const auto c : cram) {
        assert(c == 0x0EEEU);
    }
    assert(res.jumped_to_gameplay);

    std::cout << "[PASS] test_smooth_crossfade_mode\n";
}

static void test_standard_fade_mode() {
    RoomPresentationConfig config;
    config.mode = WarpPresentationMode::StandardFade;

    std::array<std::uint16_t, 64> cram{};
    std::array<std::uint16_t, 64> scratch{};
    std::array<std::uint16_t, 64> target_white{};

    const auto res = execute_room_presentation(config, cram, scratch, target_white);

    assert(res.standard_fade_called);
    assert(!res.instant_commit_called);
    assert(!res.footsteps_played);
    assert(res.sound_commands_emitted.empty());

    std::cout << "[PASS] test_standard_fade_mode\n";
}

static void test_instant_footsteps_mode() {
    RoomPresentationConfig config;
    config.mode = WarpPresentationMode::InstantFootsteps;

    std::array<std::uint16_t, 64> cram{};
    std::array<std::uint16_t, 64> scratch{};
    std::array<std::uint16_t, 64> target_white{};

    const auto res = execute_room_presentation(config, cram, scratch, target_white);

    assert(res.instant_commit_called);
    assert(res.footsteps_played);
    assert(res.sound_commands_emitted.size() == 4U);
    assert(res.sound_commands_emitted[0] == 0x0090038EU);
    assert(res.sound_commands_emitted[1] == 0x0090038FU);
    assert(res.sound_commands_emitted[2] == 0x0090038EU);
    assert(res.sound_commands_emitted[3] == 0x0090038FU);
    assert(res.frame_delays.size() == 4U);
    for (const auto d : res.frame_delays) {
        assert(d == 14U);
    }

    std::cout << "[PASS] test_instant_footsteps_mode\n";
}

static void test_post_fade_room_flag_and_sfx() {
    // 1. With exit SFX trigger
    RoomPresentationConfig cfg1;
    cfg1.mode = WarpPresentationMode::StandardFade;
    cfg1.trigger_exit_sfx = true;
    cfg1.suppress_exit_sfx = false;
    cfg1.room_flag_bit1 = true;

    std::array<std::uint16_t, 64> cram{};
    std::array<std::uint16_t, 64> scratch{};
    std::array<std::uint16_t, 64> white{};

    const auto res1 = execute_room_presentation(cfg1, cram, scratch, white);
    assert(res1.room_flag_164f_set);
    assert(res1.sound_commands_emitted.size() == 1U);
    assert(res1.sound_commands_emitted[0] == 0x00800230U);

    // 2. With SFX suppression
    RoomPresentationConfig cfg2;
    cfg2.mode = WarpPresentationMode::StandardFade;
    cfg2.trigger_exit_sfx = true;
    cfg2.suppress_exit_sfx = true;

    const auto res2 = execute_room_presentation(cfg2, cram, scratch, white);
    assert(res2.sound_commands_emitted.empty());
    assert(res2.sfx_suppress_cleared);

    std::cout << "[PASS] test_post_fade_room_flag_and_sfx\n";
}

static void test_copy_cram_palette_line1() {
    std::array<std::uint16_t, 32> src{};
    for (std::size_t i = 0; i < 32; ++i) {
        src[i] = static_cast<std::uint16_t>(0x0246U + i * 2U);
    }
    std::array<std::uint16_t, 32> dst{};

    copy_cram_palette_line1(src, dst);

    for (std::size_t i = 0; i < 32; ++i) {
        assert(dst[i] == src[i]);
    }

    std::cout << "[PASS] test_copy_cram_palette_line1\n";
}

static void test_genesis_color_interpolation() {
    // Step 0: pure source
    const std::uint16_t c0 = interpolate_genesis_color(0x0246U, 0x0EEEU, 0U, 32U);
    assert(c0 == 0x0246U);

    // Step 32: pure destination
    const std::uint16_t c32 = interpolate_genesis_color(0x0246U, 0x0EEEU, 32U, 32U);
    assert(c32 == 0x0EEEU);

    // Intermediate step: strictly clamped to Genesis 9-bit format
    const std::uint16_t c16 = interpolate_genesis_color(0x0000U, 0x0EEEU, 16U, 32U);
    assert((c16 & 0xF000U) == 0U); // Unused upper nibble
    assert((c16 & 0x0001U) == 0U); // Unused bit 0
    assert((c16 & 0x0010U) == 0U); // Unused bit 4
    assert((c16 & 0x0100U) == 0U); // Unused bit 8

    std::cout << "[PASS] test_genesis_color_interpolation\n";
}

static void test_raw_memory_window_integration() {
    std::vector<std::uint8_t> ram(0x10000, 0x00);
    std::vector<std::uint8_t> rom(0x60000, 0x00);

    // Fill white palette at 0x05D686 in mock ROM
    for (std::size_t i = 0; i < 64; ++i) {
        write_be16_vec(rom, 0x05D686U + i * 2U, 0x0EEEU);
    }

    // Set saved SP at $FF1930
    write_be32_vec(ram, RoomPresentationJingleConfig::kSavedStackPointerAddr & 0xFFFFU, 0x00FFFD80U);

    // Test 1: Whiteout Flash in RAM
    ram[RoomPresentationJingleConfig::kSpecialFadeModeAddr & 0xFFFFU] = 1U;
    const auto res_flash = execute_room_presentation_manager_raw(ram, rom);
    assert(res_flash.whiteout_applied);
    assert(ram[RoomPresentationJingleConfig::kSpecialFadeModeAddr & 0xFFFFU] == 0U);
    assert((ram[RoomPresentationJingleConfig::kCramDirtyFlagsAddr & 0xFFFFU] & 0x01U) != 0U);

    // Test 2: Instant footsteps in RAM
    ram[RoomPresentationJingleConfig::kSpecialFadeModeAddr & 0xFFFFU] = 0U;
    ram[RoomPresentationJingleConfig::kWarpModeTypeAddr & 0xFFFFU] = 1U;
    ram[RoomPresentationJingleConfig::kWarpSfxTriggerAddr & 0xFFFFU] = 1U;
    ram[RoomPresentationJingleConfig::kRoomCustomFlagAddr & 0xFFFFU] = 0x02U; // bit 1 set

    const auto res_steps = execute_room_presentation_manager_raw(ram, rom);
    assert(res_steps.instant_commit_called);
    assert(res_steps.footsteps_played);
    assert(res_steps.sound_commands_emitted.size() == 5U); // 4 footsteps + exit chime
    assert(res_steps.sound_commands_emitted[4] == 0x00800230U);
    assert(ram[RoomPresentationJingleConfig::kFadeTimerAddr & 0xFFFFU] == 4U);
    assert(ram[RoomPresentationJingleConfig::kRoomFlagOutAddr & 0xFFFFU] == 0xFFU);

    // Test 3: Palette line 1 copy in RAM
    constexpr std::uint32_t kSourceAddr = 0x00FF8000U;
    write_be32_vec(ram, RoomPresentationJingleConfig::kSourcePalettePtrAddr & 0xFFFFU, kSourceAddr);
    for (std::size_t i = 0; i < 32; ++i) {
        write_be16_vec(ram, (kSourceAddr & 0xFFFFU) + i * 2U, static_cast<std::uint16_t>(0x0111U + i));
    }
    copy_cram_palette_line1_raw(ram);
    for (std::size_t i = 0; i < 32; ++i) {
        assert(read_be16(ram, (RoomPresentationJingleConfig::kCramPaletteLine1Addr & 0xFFFFU) + i * 2U) == (0x0111U + i));
    }

    std::cout << "[PASS] test_raw_memory_window_integration\n";
}

static void test_cleanroom_namespace_aliases() {
    thor::environment::RoomPresentationConfig config;
    config.mode = thor::environment::WarpPresentationMode::StandardFade;

    std::array<std::uint16_t, 64> cram{};
    std::array<std::uint16_t, 64> scratch{};
    std::array<std::uint16_t, 64> white{};

    const auto res = thor::world::execute_room_presentation(config, cram, scratch, white);
    assert(res.standard_fade_called);

    std::vector<std::uint32_t> snds;
    std::vector<std::uint8_t> delays;
    thor::environment::play_footsteps_jingle(snds, delays);
    assert(snds.size() == 4U);

    std::cout << "[PASS] test_cleanroom_namespace_aliases\n";
}

int main() {
    std::cout << "Starting M16.183 Room Entry Presentation Manager tests...\n";
    test_rom_constants_and_layout();
    test_whiteout_flash_mode();
    test_smooth_crossfade_mode();
    test_standard_fade_mode();
    test_instant_footsteps_mode();
    test_post_fade_room_flag_and_sfx();
    test_copy_cram_palette_line1();
    test_genesis_color_interpolation();
    test_raw_memory_window_integration();
    test_cleanroom_namespace_aliases();
    std::cout << "All M16.183 Room Entry Presentation Manager tests PASS!\n";
    return 0;
}
