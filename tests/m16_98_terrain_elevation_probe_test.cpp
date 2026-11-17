#include "game/world/terrain_elevation_probe.hpp"

#include <array>
#include <cassert>

namespace {

using namespace oasis::game::world;

void test_gray_code_lut() {
    constexpr std::array<std::int8_t, 16> expected = {
        -1, 0, 2, 1, 4, -1, 3, -1,
         6, 7, -1, -1, 5, -1, -1, -1
    };
    assert(kElevationNibbleLut16 == expected);
}

void test_passable_and_blocked_cases() {
    constexpr std::array<std::uint16_t, 8> kHeights = {0U, 16U, 32U, 48U, 64U, 80U, 96U, 112U};

    // Passable same-elevation tiles (nibble 1 -> elev 0)
    const std::array<std::uint8_t, 3> flat_tiles = {0x01U, 0x01U, 0x01U};
    ElevationProbeInput in{};
    in.entity_elev_70 = 0;
    in.span_tiles = flat_tiles;
    in.height_table = kHeights;

    const auto pass = evaluate_terrain_elevation_probe(in);
    assert(!pass.blocked && pass.flags == 0U && pass.tile_elev == 0 && pass.delta == 0);

    // Invalid/mixed elevation blocking (0x01 & 0x02 = 0x00 -> LUT[0] = -1 -> bit 6)
    const std::array<std::uint8_t, 2> mixed_tiles = {0x01U, 0x02U};
    in.span_tiles = mixed_tiles;
    const auto mixed = evaluate_terrain_elevation_probe(in);
    assert(mixed.blocked && mixed.flags == kProbeFlagBlockedBit6 && mixed.tile_elev == -1);

    // Step-up blocking (delta == +1, entity_flags & 0x20 -> bit 5)
    const std::array<std::uint8_t, 2> step_tiles = {0x03U, 0x03U}; // nibble 3 -> elev 1
    in.span_tiles = step_tiles;
    in.entity_flags_38 = 0x20U;
    const auto step_blk = evaluate_terrain_elevation_probe(in);
    assert(step_blk.blocked && step_blk.flags == kProbeFlagLedgeOrStepBit5 && step_blk.delta == 1);

    // Ledge-drop (entity_elev = 2 -> height 32, tile_elev = 0 -> height 0, delta = -2)
    in.entity_flags_38 = 0x10U;
    in.entity_elev_70 = 2;
    in.z_vel_10 = 0;
    in.span_tiles = flat_tiles;
    in.ledge_scan_depth_px = 16U;
    const auto ledge = evaluate_terrain_elevation_probe(in);
    assert(ledge.blocked);
    assert(ledge.flags == kProbeFlagLedgeOrStepBit5);
    assert(ledge.delta == -2);
    assert(ledge.drop_height == 32U);

    // Bypass flag (entity_flags & 0x01) passes even over invalid tiles
    in.entity_flags_38 = 0x01U;
    in.span_tiles = mixed_tiles;
    const auto bypass = evaluate_terrain_elevation_probe(in);
    assert(!bypass.blocked && bypass.flags == 0U);
}

} // namespace

int main() {
    test_gray_code_lut();
    test_passable_and_blocked_cases();
    return 0;
}
