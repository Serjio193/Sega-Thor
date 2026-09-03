#include "game/world/terrain_collision.hpp"

#include <array>
#include <cassert>
#include <cstdint>

int main() {
    using oasis::game::world::TerrainGateInput;
    using oasis::game::world::TerrainGateResult;
    using oasis::game::world::evaluate_terrain_gate;
    using oasis::game::world::terrain_state_from_code;

    constexpr std::array<std::int8_t, 16> expected_states{
        -1, 0, 2, 1, 4, -1, 3, -1,
         6, 7, -1, -1, 5, -1, -1, -1,
    };
    for (std::uint8_t code = 0; code < expected_states.size(); ++code) {
        assert(terrain_state_from_code(code) == expected_states[code]);
    }

    auto input = TerrainGateInput{0, 2, 2, 0};
    assert(evaluate_terrain_gate(input) == TerrainGateResult::allowed);

    input.prospective_state = -1;
    assert(evaluate_terrain_gate(input) == TerrainGateResult::blocked);

    input = TerrainGateInput{0, 2, 4, 0};
    assert(evaluate_terrain_gate(input) == TerrainGateResult::blocked);
    input = TerrainGateInput{0, 4, 2, 0};
    assert(evaluate_terrain_gate(input) == TerrainGateResult::blocked);

    input = TerrainGateInput{0, 2, 3, 0};
    assert(evaluate_terrain_gate(input) == TerrainGateResult::allowed);
    input.entity_flags = static_cast<std::uint8_t>(1U << 5U);
    assert(evaluate_terrain_gate(input) == TerrainGateResult::blocked);

    input = TerrainGateInput{0, 3, 3, static_cast<std::uint8_t>(1U << 4U)};
    assert(evaluate_terrain_gate(input) == TerrainGateResult::blocked);

    input = TerrainGateInput{static_cast<std::uint8_t>(1U << 6U), 3, 3,
                             static_cast<std::uint8_t>(1U << 7U)};
    assert(evaluate_terrain_gate(input) == TerrainGateResult::blocked);
    input.entity_flags = 0;
    assert(evaluate_terrain_gate(input) == TerrainGateResult::allowed);

    input = TerrainGateInput{0, -1, -1, 0xFFU};
    assert(evaluate_terrain_gate(input) == TerrainGateResult::allowed);
    input = TerrainGateInput{1U, 3, -1, 0xFFU};
    assert(evaluate_terrain_gate(input) == TerrainGateResult::allowed);

    return 0;
}
