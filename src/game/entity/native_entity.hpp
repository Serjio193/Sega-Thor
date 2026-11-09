#pragma once

#include <cstdint>

namespace oasis::game {

// Authentic non-player entity in scene.
struct NativeEntity {
    std::uint16_t type_id{0x0060};          // Entity type in ROM (0x0060 = beach chest)
    std::int32_t world_x{256};              // World X coordinate
    std::int32_t world_y{88};               // World Y coordinate
    std::uint8_t state{0};                  // Entity state (0 = closed, 0x6A = opened)
    std::uint8_t anim_frame{0};             // Current animation frame
    std::uint16_t tile_base{0x04A8};        // SAT base tile index
    std::int16_t hp{20};                    // Health points
    bool active{true};                      // Whether entity is active
};

} // namespace oasis::game
