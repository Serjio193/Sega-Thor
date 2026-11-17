#include "game/player/special_move_inputs.hpp"

#include <algorithm>

namespace oasis::game::player {

namespace {

struct FbfFacingSpec {
    std::array<std::uint8_t, 4> seq;
    std::array<std::uint8_t, 4> mask;
    std::uint8_t perp_mask;
};

inline constexpr std::array<FbfFacingSpec, 4> kFbfSpecs = {{
    {{1U, 2U, 1U, 0U}, {15U, 2U, 15U, 0U}, 0x0CU}, // Facing 0 (Up): perp Left/Right (4|8)
    {{8U, 4U, 8U, 0U}, {15U, 4U, 15U, 0U}, 0x03U}, // Facing 1 (Right): perp Up/Down (1|2)
    {{2U, 1U, 2U, 0U}, {15U, 1U, 15U, 0U}, 0x0CU}, // Facing 2 (Down): perp Left/Right (4|8)
    {{4U, 8U, 4U, 0U}, {15U, 8U, 15U, 0U}, 0x03U}, // Facing 3 (Left): perp Up/Down (1|2)
}};

} // namespace

bool detect_spin_360(video::InputHistoryRing64& ring) noexcept {
    const std::size_t limit =
        std::min<std::size_t>(ring.count(), SpecialMoveInputConfig::kMaxHistoryFrames);
    if (limit < 4U) {
        return false;
    }

    bool has_prev = false;
    std::uint8_t prev = 0U;
    std::int8_t dir = 0; // +2 CW, -2 CCW, 0 undecided
    std::uint8_t steps = 0U;

    for (std::size_t ago = 0U; ago < limit; ++ago) {
        const auto word = ring.frames_ago(ago);
        if (!word.has_value()) {
            break;
        }
        const std::uint8_t mapped = SpecialMoveInputConfig::kSpin360DpadLut[*word & 0x0FU];
        if (mapped == 0xFFU || (mapped & 1U) != 0U) {
            continue; // Ignore neutral (0xFF) and odd diagonals
        }

        if (!has_prev) {
            has_prev = true;
            prev = mapped;
            steps = 1U;
            dir = 0;
            continue;
        }
        if (mapped == prev) {
            continue;
        }

        const std::uint8_t cw_next = static_cast<std::uint8_t>((prev + 2U) & 7U);
        const std::uint8_t ccw_next = static_cast<std::uint8_t>((prev + 6U) & 7U);
        if (dir == 0) {
            if (mapped == cw_next) {
                dir = 2;
                steps = 2U;
                prev = mapped;
            } else if (mapped == ccw_next) {
                dir = -2;
                steps = 2U;
                prev = mapped;
            } else {
                prev = mapped;
                steps = 1U;
            }
        } else {
            const std::uint8_t expected = static_cast<std::uint8_t>((prev + (dir > 0 ? 2U : 6U)) & 7U);
            if (mapped == expected) {
                ++steps;
                prev = mapped;
                if (steps >= 4U) {
                    ring.clear_count();
                    return true;
                }
            } else if (mapped == cw_next || mapped == ccw_next) {
                dir = (mapped == cw_next) ? 2 : -2;
                steps = 2U;
                prev = mapped;
            } else {
                dir = 0;
                steps = 1U;
                prev = mapped;
            }
        }
    }
    return false;
}

bool detect_fbf_lunge(video::InputHistoryRing64& ring, std::uint8_t facing) noexcept {
    const auto& spec = kFbfSpecs[facing & 3U];
    const std::size_t limit =
        std::min<std::size_t>(ring.count(), SpecialMoveInputConfig::kMaxHistoryFrames);
    std::size_t stage = 0U;

    for (std::size_t ago = 0U; ago < limit; ++ago) {
        const auto word = ring.frames_ago(ago);
        if (!word.has_value()) {
            break;
        }
        const std::uint8_t dpad = static_cast<std::uint8_t>(*word & 0x0FU);
        if ((dpad & spec.perp_mask) != 0U) {
            return false; // Perpendicular input aborts F-B-F lunge
        }
        if ((dpad & spec.mask[stage]) == spec.seq[stage]) {
            ++stage;
            if (spec.seq[stage] == 0U) {
                ring.clear_count();
                return true;
            }
        }
    }
    return false;
}

} // namespace oasis::game::player
