#include "game/world/terrain_collision.hpp"

#include <array>

namespace oasis::game::world {
namespace {

constexpr std::array<std::int8_t, 16> kTerrainStates{
    -1, 0, 2, 1, 4, -1, 3, -1,
     6, 7, -1, -1, 5, -1, -1, -1,
};

constexpr std::uint8_t kBypassGate = 1U << 0U;
constexpr std::uint8_t kRejectStateChange = 1U << 5U;
constexpr std::uint8_t kRejectFlaggedSameState = 1U << 6U;
constexpr std::uint8_t kProspectiveBit4 = 1U << 4U;
constexpr std::uint8_t kProspectiveBit7 = 1U << 7U;

} // namespace

std::int8_t terrain_state_from_code(std::uint8_t terrain_code) noexcept {
    return kTerrainStates[terrain_code & 0x0FU];
}

TerrainGateResult evaluate_terrain_gate(TerrainGateInput input) noexcept {
    if ((input.entity_flags & kBypassGate) != 0U || input.current_state < 0) {
        return TerrainGateResult::allowed;
    }
    if (input.prospective_state < 0) {
        return TerrainGateResult::blocked;
    }

    const auto difference = static_cast<int>(input.prospective_state) -
                            static_cast<int>(input.current_state);
    if (difference >= 2 || difference <= -2) {
        return TerrainGateResult::blocked;
    }
    if (difference != 0 && (input.entity_flags & kRejectStateChange) != 0U) {
        return TerrainGateResult::blocked;
    }
    if (difference == 0 &&
        (input.prospective_any_bits & kProspectiveBit7) != 0U &&
        (input.entity_flags & kRejectFlaggedSameState) != 0U) {
        return TerrainGateResult::blocked;
    }
    if ((input.prospective_any_bits & kProspectiveBit4) != 0U) {
        return TerrainGateResult::blocked;
    }
    return TerrainGateResult::allowed;
}

} // namespace oasis::game::world
