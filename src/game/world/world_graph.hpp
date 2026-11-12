#pragma once

#include "game/world/screen_descriptor.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace oasis::game::world {

enum class TransitionEdgeType : std::uint8_t {
    BoundaryNorth = 0,
    BoundarySouth,
    TileBoxWarp,
    DoorWarp,
    DoorExit,
    CrossAreaWarp,
};

struct WorldTransitionEdge {
    ScreenId from_screen{};
    ScreenId to_screen{};
    TransitionEdgeType type{TransitionEdgeType::BoundaryNorth};
    std::uint32_t rom_branch_pc{0};
    std::int32_t trigger_y{-1};       // Y threshold (<= trigger_y for North, >= trigger_y for South/Exit)
    bool trigger_y_lte{true};          // true if <= trigger_y, false if >= trigger_y
    std::uint8_t tile_x_min{0};        // for TileBoxWarp / DoorWarp
    std::uint8_t tile_y_min{0};
    std::uint8_t tile_x_max{0};
    std::uint8_t tile_y_max{0};
    std::int16_t required_facing{-1};  // -1 for any facing, 0 for Up
    std::int32_t entry_player_x{0};
    std::int32_t entry_player_y{0};
    int initial_camera_x{0};
    int initial_camera_y{0};
    int max_camera_x{0};
    int max_camera_y{288};
    int initial_vscroll_a{136};
};

struct ScreenTransitionTarget {
    ScreenId destination_screen{};
    std::int32_t player_x{0};
    std::int32_t player_y{0};
    int camera_x{0};
    int camera_y{0};
    int max_camera_x{0};
    int max_camera_y{288};
    int vscroll_a{136};
    std::uint32_t music_stop_cmd{0x00000002U};
    TransitionEdgeType type{TransitionEdgeType::BoundaryNorth};
};

// Returns the full list of verified Group 1 world graph transition edges (intra-group and cross-area).
[[nodiscard]] std::span<const WorldTransitionEdge> get_world_edges() noexcept;
[[nodiscard]] std::span<const WorldTransitionEdge> get_group1_world_edges() noexcept;

// Evaluates whether player position and facing in current_screen triggers a transition edge.
[[nodiscard]] std::optional<ScreenTransitionTarget> evaluate_world_transition(
    ScreenId current_screen,
    std::int32_t player_x,
    std::int32_t player_y,
    std::uint16_t player_facing,
    bool locks_active = false,
    bool story_flag_3_active = false) noexcept;

// Look up default camera bounds for Group 1 screens.
void get_group1_screen_bounds(ScreenId screen_id, int& out_max_cx, int& out_max_cy) noexcept;

} // namespace oasis::game::world
