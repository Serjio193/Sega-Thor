#pragma once

#include <cstdint>

namespace oasis::game::world {

struct TerrainGateInput {
    std::uint8_t entity_flags{};
    std::int8_t current_state{-1};
    std::int8_t prospective_state{-1};
    std::uint8_t prospective_any_bits{};
};

enum class TerrainGateResult : std::uint8_t {
    allowed,
    blocked,
};

[[nodiscard]] std::int8_t terrain_state_from_code(std::uint8_t terrain_code) noexcept;

// Boolean movement result of the original directional gate at 0x938E.
// Side effects such as collision-event flags and FF196E are intentionally
// outside this pure query.
[[nodiscard]] TerrainGateResult evaluate_terrain_gate(TerrainGateInput input) noexcept;

} // namespace oasis::game::world
