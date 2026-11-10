#pragma once

#include <cstdint>

namespace oasis::game {

// Authentic non-player entity in scene (ROM entity slot size 0xBC / 188 bytes).
struct NativeEntity {
    std::uint16_t type_id{0x0060};          // Entity type in ROM (0x0060 = beach chest, 0x0030 = guard)
    std::int32_t world_x{256};              // World X coordinate (+0x08)
    std::int32_t world_y{88};               // World Y coordinate (+0x0C)
    std::int16_t world_z{0};                // World Z coordinate / elevation (+0x10)
    std::uint16_t facing{0};                // Facing direction code (+0x16: 0=Up, 1=Right, 2=Down, 3=Left)
    std::uint8_t state{0};                  // Entity state (+0x04: 0 = closed, 0x6A = opened, 0x0A = hurt)
    std::uint8_t anim_frame{0};             // Current animation ID (+0x2A)
    std::uint16_t anim_step{0};             // Animation step timer (+0x2E)
    std::uint16_t tile_base{0x04A8};        // SAT base tile index
    std::int16_t hp{50};                    // Health points (+0x8A)
    std::int16_t max_hp{50};                // Maximum health points (+0x88)
    std::int16_t defense{0};                // Defense rating (+0x92)
    std::int16_t half_width{15};            // Collision bounding half-width (+0x42)
    std::int16_t half_depth{28};            // Collision bounding half-depth (+0x44)
    std::int16_t height{32};                // Collision bounding height (+0x4A)
    std::uint16_t previous_state{0};        // Preserved pre-hit state (+0xAA)
    std::uint16_t previous_anim{0};         // Preserved pre-hit animation (+0xAC)
    bool invulnerable{false};               // Invulnerability flag (bit 3 of +0x38)
    std::uint8_t action_timer{0};           // Action timer decremented by 0x00E682 (+0x82)
    std::uint8_t flash_timer{0};            // Hit flash timer (+0x83)
    bool active{true};                      // Whether entity slot is active (>0 at +0x00)
};

} // namespace oasis::game
