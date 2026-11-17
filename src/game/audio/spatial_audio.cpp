#include "game/audio/spatial_audio.hpp"

#include <algorithm>
#include <cstdlib>

namespace oasis::game::audio {

SpatialAudioResult compute_spatial_sfx(
    std::uint16_t sound_id,
    std::int16_t dx,
    std::int16_t dy) noexcept {
    if (sound_id < kSpatialMinSoundId) {
        return SpatialAudioResult{true, 0U, 0U};
    }

    const auto adx = static_cast<std::uint16_t>(std::abs(static_cast<int>(dx)));
    const auto ady = static_cast<std::uint16_t>(std::abs(static_cast<int>(dy)));
    const std::uint16_t dist = std::max(adx, static_cast<std::uint16_t>(ady + (ady >> 2U)));

    if (sound_id >= kHiresMinSoundId) {
        const std::uint16_t eff_dist = (dist >= kHiresAttenuateStartPx)
            ? static_cast<std::uint16_t>(dist - kHiresAttenuateStartPx)
            : 0U;
        const auto drop = static_cast<std::uint8_t>((eff_dist >> 5U) + 1U);
        if (drop > kHiresMaxDropSteps) {
            return SpatialAudioResult{false, 0U, 0U};
        }
        std::uint8_t pan = 0U;
        if (adx >= static_cast<std::uint16_t>(kHiresPanDxPx)) {
            pan = (dx < 0) ? kPanLeftFlag : kPanRightFlag;
        }
        return SpatialAudioResult{true, drop, pan};
    }

    if (dist >= kFmCullDistPx) {
        return SpatialAudioResult{false, 0U, 0U};
    }

    std::uint8_t atten = 0U;
    if (dist >= kFmStep1DistPx) {
        atten++;
    }
    if (dist >= kFmStep2DistPx) {
        atten++;
    }

    std::uint8_t pan = 0U;
    if (adx >= static_cast<std::uint16_t>(kFmPanDxPx)) {
        pan = (dx < 0) ? kPanLeftFlag : kPanRightFlag;
    }
    return SpatialAudioResult{true, atten, pan};
}

SpatialCommandResult apply_spatial_audio(
    std::uint32_t sound_cmd,
    std::int16_t dx,
    std::int16_t dy) noexcept {
    const auto sound_id = static_cast<std::uint16_t>(sound_cmd & 0xFFFFU);
    if (sound_id < kSpatialMinSoundId) {
        return SpatialCommandResult{true, sound_cmd};
    }

    const auto res = compute_spatial_sfx(sound_id, dx, dy);
    if (!res.should_play) {
        return SpatialCommandResult{false, 0U};
    }

    std::uint32_t cmd = sound_cmd;
    if (sound_id >= kHiresMinSoundId) {
        std::uint8_t b2 = static_cast<std::uint8_t>((cmd >> 16U) & 0xFFU);
        b2 = static_cast<std::uint8_t>((b2 - res.attenuation_steps) & 0x1FU);
        if (res.pan_flag == kPanLeftFlag) {
            b2 = static_cast<std::uint8_t>(b2 + 0x42U);
        } else if (res.pan_flag == kPanRightFlag) {
            b2 = static_cast<std::uint8_t>(b2 + 0x82U);
        }
        cmd = (cmd & 0xFF00FFFFU) | (static_cast<std::uint32_t>(b2) << 16U);
        return SpatialCommandResult{true, cmd};
    }

    std::uint8_t b2 = res.attenuation_steps;
    if (res.pan_flag != 0U) {
        if (b2 != 0U) {
            b2--;
        }
        b2 = static_cast<std::uint8_t>(b2 | res.pan_flag);
    }
    cmd = (cmd & 0xFF00FFFFU) | (static_cast<std::uint32_t>(b2) << 16U);
    return SpatialCommandResult{true, cmd};
}

SpatialCommandResult play_spatial_at_xy(
    std::uint32_t sound_cmd,
    std::int16_t x,
    std::int16_t y,
    std::int16_t player_x,
    std::int16_t player_y) noexcept {
    const auto dx = static_cast<std::int16_t>(x - player_x);
    const auto dy = static_cast<std::int16_t>(y - player_y);
    return apply_spatial_audio(sound_cmd, dx, dy);
}

SpatialCommandResult play_spatial_at_entity(
    std::uint32_t sound_cmd,
    std::int16_t entity_x,
    std::int16_t entity_y,
    std::int16_t player_x,
    std::int16_t player_y) noexcept {
    return play_spatial_at_xy(sound_cmd, entity_x, entity_y, player_x, player_y);
}

} // namespace oasis::game::audio
