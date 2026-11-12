#include "game/world/world_graph.hpp"

#include <array>

namespace oasis::game::world {

namespace {

constexpr std::array<WorldTransitionEdge, 18> kGroup1Edges = {{
    // Screen(1,0) South Dock
    {ScreenId{1, 0}, ScreenId{1, 1}, TransitionEdgeType::BoundaryNorth,
     0x02E5C0U, 108, true, 0, 0, 0, 0, -1, 160, 296, 0, 96, 0, 96, 136},

    // Screen(1,1) Lower Beach
    {ScreenId{1, 1}, ScreenId{1, 2}, TransitionEdgeType::BoundaryNorth,
     0x02E65EU, 108, true, 0, 0, 0, 0, -1, 256, 488, 96, 288, 192, 288, 136},
    {ScreenId{1, 1}, ScreenId{1, 0}, TransitionEdgeType::BoundarySouth,
     0x02E664U, 308, false, 0, 0, 0, 0, -1, 160, 120, 0, 8, 0, 96, 136},
    {ScreenId{1, 1}, ScreenId{1, 2}, TransitionEdgeType::TileBoxWarp,
     0x02E672U, -1, true, 6, 14, 11, 14, 0, 168, 488, 72, 288, 192, 288, 136},
    {ScreenId{1, 1}, ScreenId{1, 2}, TransitionEdgeType::TileBoxWarp,
     0x02E686U, -1, true, 28, 14, 33, 14, 0, 344, 488, 184, 288, 192, 288, 136},

    // Screen(1,2) Beach / Village South
    {ScreenId{1, 2}, ScreenId{1, 3}, TransitionEdgeType::BoundaryNorth,
     0x02E784U, 76, true, 0, 0, 0, 0, -1, 160, 488, 0, 288, 0, 288, 136},
    {ScreenId{1, 2}, ScreenId{1, 1}, TransitionEdgeType::BoundarySouth,
     0x02E78AU, 500, false, 0, 0, 0, 0, -1, 160, 120, 0, 8, 0, 96, 136},
    {ScreenId{1, 2}, ScreenId{1, 8}, TransitionEdgeType::DoorWarp,
     0x02E7ACU, -1, true, 18, 30, 23, 30, 0, 160, 232, 0, 0, 0, 32, 136},

    // Screen(1,8) Village House Interior
    {ScreenId{1, 8}, ScreenId{1, 2}, TransitionEdgeType::DoorExit,
     0x02F150U, 240, false, 0, 0, 0, 0, -1, 168, 264, 72, 128, 192, 288, 136},

    // Screen(1,3) Town Combat Plaza
    {ScreenId{1, 3}, ScreenId{1, 4}, TransitionEdgeType::BoundaryNorth,
     0x02E920U, 108, true, 0, 0, 0, 0, -1, 160, 488, 0, 288, 0, 288, 136},
    {ScreenId{1, 3}, ScreenId{1, 2}, TransitionEdgeType::BoundarySouth,
     0x02E926U, 500, false, 0, 0, 0, 0, -1, 256, 88, 96, 3, 192, 288, 3},

    // Screen(1,4) Town North
    {ScreenId{1, 4}, ScreenId{1, 5}, TransitionEdgeType::BoundaryNorth,
     0x02EAA6U, 108, true, 0, 0, 0, 0, -1, 256, 296, 96, 96, 192, 96, 136},
    {ScreenId{1, 4}, ScreenId{1, 3}, TransitionEdgeType::BoundarySouth,
     0x02EAAAU, 500, false, 0, 0, 0, 0, -1, 160, 120, 0, 8, 0, 288, 136},

    // Screen(1,5) Upper Town / Boss Gate
    {ScreenId{1, 5}, ScreenId{1, 7}, TransitionEdgeType::BoundaryNorth,
     0x02ECE2U, 108, true, 0, 0, 0, 0, -1, 160, 232, 0, 32, 0, 32, 136},
    {ScreenId{1, 5}, ScreenId{1, 4}, TransitionEdgeType::BoundarySouth,
     0x02ECE8U, 308, false, 0, 0, 0, 0, -1, 160, 120, 0, 8, 0, 288, 136},

    // Screen(1,7) Shrine / Cliff Approach
    {ScreenId{1, 7}, ScreenId{1, 6}, TransitionEdgeType::BoundaryNorth,
     0x02F0C4U, 108, true, 0, 0, 0, 0, -1, 160, 296, 0, 96, 0, 96, 136},
    {ScreenId{1, 7}, ScreenId{1, 5}, TransitionEdgeType::BoundarySouth,
     0x02F0CAU, 244, false, 0, 0, 0, 0, -1, 256, 120, 96, 8, 192, 96, 136},

    // Screen(1,6) Cliff Peak / Boss Overlook
    {ScreenId{1, 6}, ScreenId{1, 7}, TransitionEdgeType::BoundarySouth,
     0x02EFE4U, 308, false, 0, 0, 0, 0, -1, 160, 120, 0, 8, 0, 32, 136},
}};

} // namespace

std::span<const WorldTransitionEdge> get_group1_world_edges() noexcept {
    return kGroup1Edges;
}

void get_group1_screen_bounds(ScreenId screen_id, int& out_max_cx, int& out_max_cy) noexcept {
    out_max_cx = 0;
    out_max_cy = 288;
    if (screen_id.group != 1U) return;
    switch (screen_id.index) {
        case 0U: case 1U: case 6U:
            out_max_cx = 0; out_max_cy = 96; break;
        case 2U:
            out_max_cx = 192; out_max_cy = 288; break;
        case 3U: case 4U:
            out_max_cx = 0; out_max_cy = 288; break;
        case 5U:
            out_max_cx = 192; out_max_cy = 96; break;
        case 7U: case 8U:
            out_max_cx = 0; out_max_cy = 32; break;
        default:
            break;
    }
}

std::optional<ScreenTransitionTarget> evaluate_world_transition(
    ScreenId current_screen,
    std::int32_t player_x,
    std::int32_t player_y,
    std::uint16_t player_facing,
    bool locks_active) noexcept {
    if (locks_active) return std::nullopt;

    const std::uint8_t tile_x = static_cast<std::uint8_t>((player_x >> 3) & 0xFF);
    const std::uint8_t tile_y = static_cast<std::uint8_t>((player_y >> 3) & 0xFF);

    for (const auto& edge : kGroup1Edges) {
        if (edge.from_screen != current_screen) continue;

        if (edge.type == TransitionEdgeType::BoundaryNorth) {
            if (player_y <= edge.trigger_y) {
                return ScreenTransitionTarget{
                    edge.to_screen, edge.entry_player_x, edge.entry_player_y,
                    edge.initial_camera_x, edge.initial_camera_y,
                    edge.max_camera_x, edge.max_camera_y, edge.initial_vscroll_a,
                    0x00000002U, edge.type};
            }
        } else if (edge.type == TransitionEdgeType::BoundarySouth || edge.type == TransitionEdgeType::DoorExit) {
            if (player_y >= edge.trigger_y) {
                return ScreenTransitionTarget{
                    edge.to_screen, edge.entry_player_x, edge.entry_player_y,
                    edge.initial_camera_x, edge.initial_camera_y,
                    edge.max_camera_x, edge.max_camera_y, edge.initial_vscroll_a,
                    0x00000002U, edge.type};
            }
        } else if (edge.type == TransitionEdgeType::TileBoxWarp || edge.type == TransitionEdgeType::DoorWarp) {
            if (edge.required_facing >= 0 && static_cast<std::int16_t>(player_facing) != edge.required_facing) {
                continue;
            }
            if (tile_x >= edge.tile_x_min && tile_x <= edge.tile_x_max &&
                tile_y >= edge.tile_y_min && tile_y <= edge.tile_y_max) {
                return ScreenTransitionTarget{
                    edge.to_screen, edge.entry_player_x, edge.entry_player_y,
                    edge.initial_camera_x, edge.initial_camera_y,
                    edge.max_camera_x, edge.max_camera_y, edge.initial_vscroll_a,
                    0x00000002U, edge.type};
            }
        }
    }

    return std::nullopt;
}

} // namespace oasis::game::world
