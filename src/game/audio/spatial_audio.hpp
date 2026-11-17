#pragma once

#include <cstdint>

namespace oasis::game::audio {

inline constexpr std::uint32_t kPlaySpatialAtXyAddr = 0x00E78EU;
inline constexpr std::uint32_t kPlaySpatialAtEntityAddr = 0x00E7E8U;

inline constexpr std::uint16_t kSpatialMinSoundId = 0x0200U;
inline constexpr std::uint16_t kHiresMinSoundId = 0x0300U;

inline constexpr std::uint16_t kHiresAttenuateStartPx = 88U;  // 0x58
inline constexpr std::uint8_t kHiresMaxDropSteps = 7U;
inline constexpr std::int16_t kHiresPanDxPx = 64;             // 0x40

inline constexpr std::uint16_t kFmStep1DistPx = 176U;         // 0xB0
inline constexpr std::uint16_t kFmStep2DistPx = 224U;         // 0xE0
inline constexpr std::uint16_t kFmCullDistPx = 272U;          // 0x110
inline constexpr std::int16_t kFmPanDxPx = 96;                // 0x60

inline constexpr std::uint8_t kPanLeftFlag = 0x40U;
inline constexpr std::uint8_t kPanRightFlag = 0x80U;

struct SpatialAudioResult {
    bool should_play{false};
    std::uint8_t attenuation_steps{0U};
    std::uint8_t pan_flag{0U}; // 0=center, 0x40=left, 0x80=right
};

struct SpatialCommandResult {
    bool should_play{false};
    std::uint32_t command{0U};
};

/// Models 0x00E7E8..0x00E90C: computes (should_play, attenuation_steps, pan_flag)
/// for sound_id relative to listener delta (dx, dy).
[[nodiscard]] SpatialAudioResult compute_spatial_sfx(
    std::uint16_t sound_id,
    std::int16_t dx,
    std::int16_t dy) noexcept;

/// Models 0x00E78E..0x00E90C: applies 2D attenuation and stereo panning to 32-bit sound command.
[[nodiscard]] SpatialCommandResult apply_spatial_audio(
    std::uint32_t sound_cmd,
    std::int16_t dx,
    std::int16_t dy) noexcept;

/// Models 0x00E78E..0x00E7E6: spatializes sound at (x, y) relative to player (player_x, player_y).
[[nodiscard]] SpatialCommandResult play_spatial_at_xy(
    std::uint32_t sound_cmd,
    std::int16_t x,
    std::int16_t y,
    std::int16_t player_x,
    std::int16_t player_y) noexcept;

/// Models 0x00E7E8..0x00E90C: spatializes sound at entity (entity_x, entity_y) relative to player.
[[nodiscard]] SpatialCommandResult play_spatial_at_entity(
    std::uint32_t sound_cmd,
    std::int16_t entity_x,
    std::int16_t entity_y,
    std::int16_t player_x,
    std::int16_t player_y) noexcept;

} // namespace oasis::game::audio
