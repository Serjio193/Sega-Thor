#pragma once

#include "game/combat/sound_command_queue.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace oasis::game::audio {

// ROM 0x03E7F4..0x03F047: Secret Sound Test menu categories.
enum class SoundTestCategory : std::uint8_t {
    Bgm = 0,
    Sfx,
    Pcm,
    Exit,
};

struct SoundTestTrackEntry {
    std::uint8_t menu_index{0U};
    std::uint8_t track_byte{0U};
    std::uint16_t bgm_cmd{0U}; // 0x0100 | track_byte
    std::string_view title{};
};

struct SoundTestConfig {
    static constexpr std::uint32_t kEntryAddr = 0x0003E7F4U;
    static constexpr std::uint32_t kUnlockChordFF165E = 0x90000000U;
    static constexpr std::uint32_t kLabelsAddr = 0x0003ED48U;
    static constexpr std::uint32_t kTrackIdMapAddr = 0x0003ED7EU;
    static constexpr std::uint32_t kTrackNamesAddr = 0x0003EDA0U;
    static constexpr std::uint8_t kTrackCount = 34U;
    static constexpr std::uint32_t kStopBgmCmd = 0x00000002U;
};

// All 34 Yuzo Koshiro OST tracks from ROM 0x03ED7E + 0x03EDA0..0x03F047.
extern const std::array<SoundTestTrackEntry, SoundTestConfig::kTrackCount> kSoundTestTracks;

// Looks up a Sound Test track by menu index (0..33). If `rom` covers 0x03ED7E,
// reads `track_byte` from ROM; otherwise uses `kSoundTestTracks`.
[[nodiscard]] std::optional<SoundTestTrackEntry> lookup_sound_test_track(
    std::span<const std::uint8_t> rom,
    std::uint8_t menu_index) noexcept;

// Models Secret Sound Test menu state (BGM / SFX / PCM / EXIT).
struct SoundTestMenuState {
    SoundTestCategory category{SoundTestCategory::Bgm};
    std::uint8_t bgm_index{0U}; // 0..33
    std::uint8_t sfx_index{0U};
    std::uint8_t pcm_index{0U};
    bool exit_requested{false};
    std::uint32_t last_emitted_cmd{0U};

    void reset() noexcept;
    void cycle_category(int delta) noexcept;
    void cycle_value(int delta) noexcept;
    void play_selected(combat::SoundCommandQueue& queue, int frame_60hz = 0);
    void stop_bgm(combat::SoundCommandQueue& queue, int frame_60hz = 0);

    [[nodiscard]] static constexpr bool is_unlock_chord(std::uint32_t input_ff165e_l) noexcept {
        return input_ff165e_l == SoundTestConfig::kUnlockChordFF165E;
    }
};

} // namespace oasis::game::audio
