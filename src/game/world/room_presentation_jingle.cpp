#include "room_presentation_jingle.hpp"

#include <algorithm>

namespace oasis::game::world {

namespace {

inline std::uint16_t read_be16(std::span<const std::uint8_t> ram, std::uint32_t addr) noexcept {
    const std::size_t off = static_cast<std::size_t>(addr & 0xFFFFU);
    if (off + 1U >= ram.size()) {
        return 0U;
    }
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(ram[off]) << 8) |
                                      static_cast<std::uint16_t>(ram[off + 1U]));
}

inline void write_be16(std::span<std::uint8_t> ram, std::uint32_t addr, std::uint16_t val) noexcept {
    const std::size_t off = static_cast<std::size_t>(addr & 0xFFFFU);
    if (off + 1U >= ram.size()) {
        return;
    }
    ram[off] = static_cast<std::uint8_t>((val >> 8) & 0xFFU);
    ram[off + 1U] = static_cast<std::uint8_t>(val & 0xFFU);
}

inline std::uint32_t read_be32(std::span<const std::uint8_t> ram, std::uint32_t addr) noexcept {
    const std::size_t off = static_cast<std::size_t>(addr & 0xFFFFU);
    if (off + 3U >= ram.size()) {
        return 0U;
    }
    return (static_cast<std::uint32_t>(ram[off]) << 24) |
           (static_cast<std::uint32_t>(ram[off + 1U]) << 16) |
           (static_cast<std::uint32_t>(ram[off + 2U]) << 8) |
           static_cast<std::uint32_t>(ram[off + 3U]);
}

inline std::uint8_t read_u8(std::span<const std::uint8_t> ram, std::uint32_t addr) noexcept {
    const std::size_t off = static_cast<std::size_t>(addr & 0xFFFFU);
    if (off >= ram.size()) {
        return 0U;
    }
    return ram[off];
}

inline void write_u8(std::span<std::uint8_t> ram, std::uint32_t addr, std::uint8_t val) noexcept {
    const std::size_t off = static_cast<std::size_t>(addr & 0xFFFFU);
    if (off >= ram.size()) {
        return;
    }
    ram[off] = val;
}

} // namespace

void play_footsteps_jingle(
    std::vector<std::uint32_t>& sound_sink,
    std::vector<std::uint8_t>& delay_sink) {
    constexpr std::uint32_t kSeq[4] = {
        RoomPresentationJingleConfig::kSndFootstepA,
        RoomPresentationJingleConfig::kSndFootstepB,
        RoomPresentationJingleConfig::kSndFootstepA,
        RoomPresentationJingleConfig::kSndFootstepB
    };

    for (const auto snd : kSeq) {
        sound_sink.push_back(snd);
        delay_sink.push_back(RoomPresentationJingleConfig::kFootstepDelayFrames);
    }
}

void copy_cram_palette_line1(
    std::span<const std::uint16_t, 32> source_palette,
    std::span<std::uint16_t, 32> dest_cram_line) noexcept {
    for (std::size_t i = 0U; i < 32U; ++i) {
        dest_cram_line[i] = source_palette[i];
    }
}

std::uint16_t interpolate_genesis_color(
    std::uint16_t src,
    std::uint16_t dst,
    std::uint8_t step,
    std::uint8_t max_steps) noexcept {
    if (max_steps == 0U) {
        return dst;
    }
    if (step == 0U) {
        return src;
    }
    if (step >= max_steps) {
        return dst;
    }

    const auto r_src = static_cast<std::int32_t>((src & 0x000EU) >> 1);
    const auto g_src = static_cast<std::int32_t>((src & 0x00E0U) >> 5);
    const auto b_src = static_cast<std::int32_t>((src & 0x0E00U) >> 9);

    const auto r_dst = static_cast<std::int32_t>((dst & 0x000EU) >> 1);
    const auto g_dst = static_cast<std::int32_t>((dst & 0x00E0U) >> 5);
    const auto b_dst = static_cast<std::int32_t>((dst & 0x0E00U) >> 9);

    const auto r_interp = std::clamp(r_src + ((r_dst - r_src) * static_cast<std::int32_t>(step)) / static_cast<std::int32_t>(max_steps), 0, 7);
    const auto g_interp = std::clamp(g_src + ((g_dst - g_src) * static_cast<std::int32_t>(step)) / static_cast<std::int32_t>(max_steps), 0, 7);
    const auto b_interp = std::clamp(b_src + ((b_dst - b_src) * static_cast<std::int32_t>(step)) / static_cast<std::int32_t>(max_steps), 0, 7);

    return static_cast<std::uint16_t>((b_interp << 9) | (g_interp << 5) | (r_interp << 1));
}

RoomPresentationResult execute_room_presentation(
    const RoomPresentationConfig& config,
    std::span<std::uint16_t, 64> cram_palette,
    std::span<std::uint16_t, 64> scratch_buffer,
    std::span<const std::uint16_t, 64> target_white_palette) noexcept {
    RoomPresentationResult result;

    if (config.mode == WarpPresentationMode::WhiteoutFlash) {
        // 0x00CDEC: fill 64 words with #$0EEE
        result.whiteout_applied = true;
        std::fill(cram_palette.begin(), cram_palette.end(), RoomPresentationJingleConfig::kPureWhiteColor);
    } else if (config.mode == WarpPresentationMode::SmoothCrossfade) {
        // 0x00CDA8: copy to scratch buffer and iterate 33 interpolation steps
        for (std::size_t i = 0U; i < 64U; ++i) {
            scratch_buffer[i] = cram_palette[i];
        }
        for (std::uint8_t step = 0U; step < RoomPresentationJingleConfig::kCrossfadeStepCount; ++step) {
            for (std::size_t c = 0U; c < 64U; ++c) {
                cram_palette[c] = interpolate_genesis_color(
                    scratch_buffer[c],
                    target_white_palette[c],
                    step,
                    32U);
            }
            ++result.crossfade_steps_completed;
        }
    } else if (config.mode == WarpPresentationMode::InstantFootsteps) {
        // 0x00CD7E: instant warp commit and footsteps
        result.instant_commit_called = true;
        play_footsteps_jingle(result.sound_commands_emitted, result.frame_delays);
        result.footsteps_played = true;
    } else {
        // 0x00CE22: standard fade-in
        result.standard_fade_called = true;
    }

    // 0x00CE2E: Post-fade finalization
    if (config.room_flag_bit1) {
        result.room_flag_164f_set = true;
    }

    if (config.trigger_exit_sfx && !config.suppress_exit_sfx) {
        result.sound_commands_emitted.push_back(RoomPresentationJingleConfig::kSndWarpExitChime);
    }

    if (config.suppress_exit_sfx) {
        result.sfx_suppress_cleared = true;
    }

    result.restored_sp = config.saved_sp;
    result.jumped_to_gameplay = true;

    return result;
}

void execute_whiteout_flash_raw(std::span<std::uint8_t> ram) noexcept {
    constexpr std::uint32_t kBase = RoomPresentationJingleConfig::kCramBufferBaseAddr;
    for (std::size_t i = 0U; i < 64U; ++i) {
        write_be16(ram, kBase + static_cast<std::uint32_t>(i * 2U), RoomPresentationJingleConfig::kPureWhiteColor);
    }
    const std::uint8_t flags = read_u8(ram, RoomPresentationJingleConfig::kCramDirtyFlagsAddr);
    write_u8(ram, RoomPresentationJingleConfig::kCramDirtyFlagsAddr, static_cast<std::uint8_t>(flags | 0x01U));
    write_u8(ram, RoomPresentationJingleConfig::kSpecialFadeModeAddr, 0U);
}

std::uint8_t execute_palette_crossfade_raw(
    std::span<std::uint8_t> ram,
    std::span<const std::uint8_t> rom) noexcept {
    constexpr std::uint32_t kBase = RoomPresentationJingleConfig::kCramBufferBaseAddr;
    constexpr std::uint32_t kScratch = RoomPresentationJingleConfig::kPaletteScratchBufferAddr;

    std::array<std::uint16_t, 64> scratch{};
    for (std::size_t i = 0U; i < 64U; ++i) {
        scratch[i] = read_be16(ram, kBase + static_cast<std::uint32_t>(i * 2U));
        write_be16(ram, kScratch + static_cast<std::uint32_t>(i * 2U), scratch[i]);
    }

    std::array<std::uint16_t, 64> target_white{};
    constexpr std::size_t kWhiteOffset = RoomPresentationJingleConfig::kTargetWhitePaletteAddr;
    if (rom.size() >= kWhiteOffset + 128U) {
        for (std::size_t i = 0U; i < 64U; ++i) {
            target_white[i] = static_cast<std::uint16_t>(
                (static_cast<std::uint16_t>(rom[kWhiteOffset + i * 2U]) << 8) |
                static_cast<std::uint16_t>(rom[kWhiteOffset + i * 2U + 1U]));
        }
    } else {
        target_white.fill(RoomPresentationJingleConfig::kPureWhiteColor);
    }

    for (std::uint8_t step = 0U; step < RoomPresentationJingleConfig::kCrossfadeStepCount; ++step) {
        for (std::size_t c = 0U; c < 64U; ++c) {
            const auto col = interpolate_genesis_color(scratch[c], target_white[c], step, 32U);
            write_be16(ram, kBase + static_cast<std::uint32_t>(c * 2U), col);
        }
    }

    write_u8(ram, RoomPresentationJingleConfig::kSpecialFadeModeAddr, 0U);
    return static_cast<std::uint8_t>(RoomPresentationJingleConfig::kCrossfadeStepCount);
}

void copy_cram_palette_line1_raw(std::span<std::uint8_t> ram) noexcept {
    const std::uint32_t src_ptr = read_be32(ram, RoomPresentationJingleConfig::kSourcePalettePtrAddr);
    constexpr std::uint32_t kDest = RoomPresentationJingleConfig::kCramPaletteLine1Addr;

    for (std::size_t i = 0U; i < 32U; ++i) {
        const std::uint16_t val = read_be16(ram, src_ptr + static_cast<std::uint32_t>(i * 2U));
        write_be16(ram, kDest + static_cast<std::uint32_t>(i * 2U), val);
    }
}

RoomPresentationResult execute_room_presentation_manager_raw(
    std::span<std::uint8_t> ram,
    std::span<const std::uint8_t> rom) noexcept {
    // 0x00CD64: wait for fade busy to clear
    write_u8(ram, RoomPresentationJingleConfig::kDmaFadeBusyAddr, 0U);

    RoomPresentationConfig config;
    const std::uint8_t special_mode = read_u8(ram, RoomPresentationJingleConfig::kSpecialFadeModeAddr);
    const std::uint8_t warp_type = read_u8(ram, RoomPresentationJingleConfig::kWarpModeTypeAddr);

    if (special_mode == 1U) {
        config.mode = WarpPresentationMode::WhiteoutFlash;
    } else if (special_mode > 1U) {
        config.mode = WarpPresentationMode::SmoothCrossfade;
    } else if (warp_type != 0U) {
        config.mode = WarpPresentationMode::InstantFootsteps;
    } else {
        config.mode = WarpPresentationMode::StandardFade;
    }

    config.room_flag_bit1 = (read_u8(ram, RoomPresentationJingleConfig::kRoomCustomFlagAddr) & 0x02U) != 0U;
    config.trigger_exit_sfx = (read_u8(ram, RoomPresentationJingleConfig::kWarpSfxTriggerAddr) != 0U);
    config.suppress_exit_sfx = (read_u8(ram, RoomPresentationJingleConfig::kWarpSfxSuppressAddr) != 0U);
    config.saved_sp = read_be32(ram, RoomPresentationJingleConfig::kSavedStackPointerAddr);

    std::array<std::uint16_t, 64> cram{};
    std::array<std::uint16_t, 64> scratch{};
    for (std::size_t i = 0U; i < 64U; ++i) {
        cram[i] = read_be16(ram, RoomPresentationJingleConfig::kCramBufferBaseAddr + static_cast<std::uint32_t>(i * 2U));
    }

    std::array<std::uint16_t, 64> target_white{};
    constexpr std::size_t kWhiteOffset = RoomPresentationJingleConfig::kTargetWhitePaletteAddr;
    if (rom.size() >= kWhiteOffset + 128U) {
        for (std::size_t i = 0U; i < 64U; ++i) {
            target_white[i] = static_cast<std::uint16_t>(
                (static_cast<std::uint16_t>(rom[kWhiteOffset + i * 2U]) << 8) |
                static_cast<std::uint16_t>(rom[kWhiteOffset + i * 2U + 1U]));
        }
    } else {
        target_white.fill(RoomPresentationJingleConfig::kPureWhiteColor);
    }

    const auto res = execute_room_presentation(config, cram, scratch, target_white);

    // Apply RAM mutations
    if (config.mode == WarpPresentationMode::WhiteoutFlash) {
        execute_whiteout_flash_raw(ram);
    } else if (config.mode == WarpPresentationMode::SmoothCrossfade) {
        execute_palette_crossfade_raw(ram, rom);
    } else if (config.mode == WarpPresentationMode::InstantFootsteps) {
        write_u8(ram, RoomPresentationJingleConfig::kFadeTimerAddr, RoomPresentationJingleConfig::kInstantWarpTimer);
        const std::uint8_t dirty = read_u8(ram, RoomPresentationJingleConfig::kCramDirtyFlagsAddr);
        write_u8(ram, RoomPresentationJingleConfig::kCramDirtyFlagsAddr, static_cast<std::uint8_t>(dirty & ~0x10U));
    } else {
        write_u8(ram, RoomPresentationJingleConfig::kFadeTimerAddr, 0U);
    }

    if (res.room_flag_164f_set) {
        write_u8(ram, RoomPresentationJingleConfig::kRoomFlagOutAddr, 0xFFU);
    }

    if (res.sfx_suppress_cleared) {
        write_u8(ram, RoomPresentationJingleConfig::kWarpSfxSuppressAddr, 0U);
    }

    return res;
}

} // namespace oasis::game::world
