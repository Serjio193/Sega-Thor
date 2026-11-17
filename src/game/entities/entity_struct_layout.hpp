#pragma once

#include <cstdint>
#include <optional>
#include <span>

namespace oasis::game::entities {

inline constexpr std::uint32_t kEntityConstructorAddr = 0x008D06U;
inline constexpr std::uint32_t kEntityReinitPointersAddr = 0x008DA4U;
inline constexpr std::uint32_t kEntityMasterDescriptorTableRom = 0x03F2FAU;
inline constexpr std::uint32_t kEntityDescriptorStrideBytes = 32U;
inline constexpr std::uint16_t kFullEntityClearWords = 0x5DU;  // 93 words (+0x02..+0xBB)
inline constexpr std::uint16_t kLightEntityClearWords = 0x2CU; // 44 words
inline constexpr std::uint32_t kLightEntityRamThreshold = 0xFF2954U;
inline constexpr std::uint16_t kEntityStructSizeBytes = 0xBCU;

/// 32-byte master entity descriptor decoded from ROM 0x03F2FA + (entity_id - 2) * 16.
struct EntityDescriptor32 {
    std::uint16_t entity_id{0};
    std::uint32_t rom_addr{0};
    std::uint16_t height_z{0};        // +0x00 (u16) -> entity +0x4A
    std::uint8_t radius_extent{0};    // +0x02 (u8)  -> entity +0x46/+0x42/+0x44
    std::uint8_t hitbox_half_w{0};    // +0x03 (u8)  -> entity +0x48
    std::uint8_t hitbox_half_h{0};    // +0x04 (u8)  -> entity +0x49
    std::uint8_t weight_flags{0};     // +0x05 (u8)  -> entity +0x3D
    std::uint8_t shadow_z_offset{0};  // +0x06 (u8)  -> entity +0x4C
    std::uint8_t trait_flags{0};      // +0x07 (u8)  -> entity +0x3A (bit0 = spirit)
    std::uint8_t collision_flags{0};  // +0x08 (u8)  -> entity +0x36
    std::uint8_t combat_flags{0};     // +0x09 (u8)  -> entity +0x37
    std::uint8_t render_flags{0};     // +0x0A (u8)  -> entity +0x38
    std::uint8_t behavior_flags{0};   // +0x0B (u8)  -> entity +0x3C
    std::uint32_t vram_tile_ptr{0};   // +0x0C (u32) -> entity +0x1A
    std::uint32_t sprite_frame_ptr{0};// +0x10 (u32) -> entity +0x1E
    std::uint32_t anim_script_ptr{0}; // +0x14 (u32) -> entity +0x26
    std::uint32_t hitbox_table_ptr{0};// +0x18 (u32) -> entity +0x22
    std::uint32_t secondary_ai_ptr{0};// +0x1C (u32) -> entity +0x5E (< $FF2954 only)
};

/// Unpacked entity constructor state produced by 0x008D06 / updated by 0x008DA4.
struct EntityConstructorState {
    std::uint16_t entity_id{0};
    bool is_lightweight_slot{false};
    std::uint16_t cleared_words{0};

    // Init timers set to 0xFFFF by 0x008D06.
    std::uint16_t timer_5c{0};
    std::uint16_t timer_2c{0};
    std::uint16_t timer_34{0};

    // 16 unpacked descriptor fields.
    std::uint16_t height_z{0};        // +0x4A
    std::uint8_t radius_extent{0};    // +0x46 (also copied to +0x42 and +0x44)
    std::uint8_t radius_42{0};        // +0x42
    std::uint8_t radius_44{0};        // +0x44
    std::uint8_t hitbox_half_w{0};    // +0x48
    std::uint8_t hitbox_half_h{0};    // +0x49
    std::uint8_t weight_flags{0};     // +0x3D
    std::uint8_t shadow_z_offset{0};  // +0x4C
    std::uint8_t trait_flags{0};      // +0x3A
    std::uint8_t collision_flags{0};  // +0x36
    std::uint8_t combat_flags{0};     // +0x37
    std::uint8_t render_flags{0};     // +0x38
    std::uint8_t behavior_flags{0};   // +0x3C
    std::uint32_t vram_tile_ptr{0};   // +0x1A
    std::uint32_t sprite_frame_ptr{0};// +0x1E
    std::uint32_t anim_script_ptr{0}; // +0x26
    std::uint32_t hitbox_table_ptr{0};// +0x22
    std::uint32_t secondary_ai_ptr{0};// +0x5E (zeroed if is_lightweight_slot)
};

[[nodiscard]] constexpr bool is_lightweight_entity_ram_addr(std::uint32_t ram_addr) noexcept {
    return ram_addr >= kLightEntityRamThreshold;
}

/// Decodes the 32-byte master entity descriptor at 0x03F2FA + (entity_id - 2) * 16.
[[nodiscard]] std::optional<EntityDescriptor32> decode_entity_descriptor32(
    std::span<const std::uint8_t> rom,
    std::uint16_t entity_id) noexcept;

/// Models universal entity constructor 0x008D06: zeroes 0x5D or 0x2C words,
/// sets timers +0x5C/+0x2C/+0x34 = 0xFFFF, and unpacks the 32-byte descriptor.
[[nodiscard]] std::optional<EntityConstructorState> init_entity_from_descriptor(
    std::span<const std::uint8_t> rom,
    std::uint16_t entity_id,
    bool is_lightweight_slot) noexcept;

/// Models 0x008DA4: re-initializes the descriptor-driven fields (pointers and
/// geometry/flags) on an existing entity slot without clearing timers/state.
[[nodiscard]] bool reinit_entity_pointers(
    std::span<const std::uint8_t> rom,
    std::uint16_t entity_id,
    EntityConstructorState& state) noexcept;

} // namespace oasis::game::entities
