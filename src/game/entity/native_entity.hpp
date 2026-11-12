#pragma once

#include <cstdint>

namespace oasis::game {

// Authentic non-player entity in scene (ROM entity slot size 0xBC / 188 bytes).
struct NativeEntity {
    std::uint16_t type_id{0x0060};          // Entity type ID (+0x00: 0x0060=chest, 0x002C=soldier, 0x0030=0x015AE2, 0x006E=0x01EB54)
    std::int32_t world_x{256};              // World X coordinate (+0x08)
    std::int32_t world_y{88};               // World Y coordinate (+0x0C)
    std::int16_t world_z{0};                // World Z coordinate / elevation (+0x10)
    std::uint16_t facing{0};                // Facing direction code (+0x16: 0=Up, 1=Right, 2=Down, 3=Left)
    std::uint8_t state{0};                  // Entity-class-specific state ID (+0x04)
    std::uint8_t anim_frame{0};             // Current animation ID (+0x2A)
    std::uint16_t anim_step{0};             // Animation step timer (+0x2E)
    std::uint16_t tile_base{0x04A8};        // SAT base tile + palette/priority bits (+0x18)
    std::int16_t hp{50};                    // Health points (+0x8A)
    std::int16_t max_hp{50};                // Maximum health points (+0x88)
    std::int16_t defense{0};                // Defense rating (+0x92)
    std::int16_t half_width{15};            // Collision bounding half-width (+0x42)
    std::int16_t half_depth{28};            // Collision bounding half-depth (+0x44)
    std::int16_t height{32};                // Collision bounding height (+0x4A)
    std::uint16_t previous_state{0};        // Preserved pre-hit state (+0xAA)
    std::uint16_t previous_anim{0};         // Preserved pre-hit animation (+0xAC)
    std::uint16_t damage_mailbox{0};        // Incoming damage mailbox (+0x86)
    std::uint8_t flags36{0};                // Render/control flags (+0x36: bit 0=dyn VRAM, bit 1=hidden, bit 2/3=shadow, bit 6=flash)
    bool invulnerable{false};               // Invulnerability/dead flag (bit 3 of +0x38)
    std::uint8_t action_timer{0};           // Action timer decremented by 0x00E682 (+0x82)
    std::uint8_t flash_timer{0};            // Hit flash timer (+0x83)
    bool active{true};                      // Whether entity slot is active (>0 at +0x00)
    std::int32_t x_fp{0};                   // 16.16 fixed-point world X (+0x08)
    std::int32_t y_fp{0};                   // 16.16 fixed-point world Y (+0x0C)
    std::int32_t z_fp{0};                   // 16.16 fixed-point world Z (+0x10)
    std::int32_t vx_fp{0};                  // 16.16 X velocity (+0x4E)
    std::int32_t vy_fp{0};                  // 16.16 Y velocity (+0x52)
    std::int32_t vz_fp{0};                  // 16.16 Z velocity (+0x56)
    std::uint8_t flags37{0};                // Anim status flags (+0x37: bit 0=vel halve, bit 1=anim_ended, bit 2=looped)
    std::uint8_t flags38{0};                // Combat flags (+0x38: bit 2=ground dead, bit 3=invuln, bit 5=variant flag)
    std::uint8_t ground_linger_ticks{0};    // Internal death landing/corpse timer (does not mutate +0x82)
    std::uint32_t last_sound_cmd{0};        // Last 32-bit Genesis Z80 sound command dispatched

    // Authentic ROM scheduler & animation stepper fields (M16.23)
    std::uint16_t slot_index{0};            // Entity pool index (0..19)
    std::uint16_t ai_timer{0};              // AI idle countdown (+0x02)
    std::uint16_t previous_anim_id{0xFFFFU}; // Previous anim ID (+0x2C)
    std::uint16_t anim_key_off{0};          // Current byte offset in anim sequence (+0x30)
    std::uint16_t frame_id{0};              // Current ROM sprite frame ID (+0x32)
    std::uint16_t frame_dir{0xFFFFU};       // Directional frame ID (+0x34)
    std::uint8_t flags39{0};                // Terrain/water overlay flags (+0x39, bit 1 = standing in water)
    std::uint8_t flags3a{0};                // Visibility/scheduler flags (+0x3A)
    std::uint8_t shadow_idx{0x10U};         // Index in 0x00ACFE shadow table (+0x3D)
    std::uint16_t overlay_3e{0xFFFFU};      // Weapon/overlay frame (+0x3E)
    std::uint8_t base_half_w{10};           // Unrotated X half-extent (+0x48)
    std::uint8_t base_half_d{7};            // Unrotated Y half-extent (+0x49)
    std::uint16_t variant_96{0};            // Behavior variant & bit 15 spawn flag (+0x96)
    std::uint16_t prng_mask_a0{0x0703U};    // PRNG idle/hop decision mask (+0xA0 from 0x025570)
    std::int16_t linked_slot_9e{-1};        // Linked thrown object slot (+0x9E / 0xBC)
    std::uint8_t water_ripple_tick{0};      // Water ripple 0..8 counter (+0x99)
    std::uint16_t message_id{0};            // Dialogue message ID for NPC interaction (+0xA0 / 0x018C28)
    std::uint8_t init_sub{0};               // Spawn init sub-mode (+0x66)
};

} // namespace oasis::game