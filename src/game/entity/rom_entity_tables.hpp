#pragma once

#include "game/entity/native_entity.hpp"

#include <cstdint>
#include <optional>
#include <span>

namespace oasis::game::entity {

// ROM 0x03F2FA: 32-byte Entity Descriptor Table (99 even types: 0x0002..0x00C6).
// Formula: addr = 0x03F2FA - 0x20 + (type_id << 4).
inline constexpr std::uint32_t kEntityDescriptorTableRom = 0x03F2FAU;
inline constexpr std::uint32_t kEntityDescriptorStride = 32U;
inline constexpr std::uint16_t kFirstDescriptorType = 0x0002U;
inline constexpr std::uint16_t kLastDescriptorType = 0x00C6U;

// ROM 0x00DF54..0x00E0B8: Entity Master Update Dispatch Table (89 even types: 0x0016..0x00C6).
// Formula: addr = 0x00DF54 - 0x2C + type_id * 2, reading 32-bit big-endian handler PC.
inline constexpr std::uint32_t kEntityUpdateTableRom = 0x00DF54U;
inline constexpr std::uint32_t kEntityUpdateEndSentinelRts = 0x00E0B8U;
inline constexpr std::uint16_t kFirstUpdateType = 0x0016U;
inline constexpr std::uint16_t kLastUpdateType = 0x00C6U;

struct RomEntityDescriptor {
    std::uint16_t type_id{0};
    std::uint32_t descriptor_addr{0};
    std::uint16_t z_radius_4a{0};   // +0x00: $4A(A6)
    std::uint8_t xy_radius_46{0};   // +0x02: $46/$42/$44(A6)
    std::uint8_t dim_x_48{0};       // +0x03: $48(A6)
    std::uint8_t dim_y_49{0};       // +0x04: $49(A6)
    std::uint8_t render_prio_3d{0}; // +0x05: $3D(A6)
    std::uint8_t mass_4c{0};        // +0x06: $4C(A6)
    std::uint8_t flags_3a{0};       // +0x07: $3A(A6)
    std::uint8_t flags_36{0};       // +0x08: $36(A6)
    std::uint8_t flags_37{0};       // +0x09: $37(A6)
    std::uint8_t flags_38{0};       // +0x0A: $38(A6)
    std::uint8_t flags_3c{0};       // +0x0B: $3C(A6)
    std::uint32_t anim_ptr_1a{0};   // +0x0C: $1A(A6)
    std::uint32_t anim_ptr_1e{0};   // +0x10: $1E(A6)
    std::uint32_t anim_ptr_26{0};   // +0x14: $26(A6)
    std::uint32_t anim_ptr_22{0};   // +0x18: $22(A6)
    std::uint32_t ai_ptr_5e{0};     // +0x1C: $5E(A6)
};

// Reads 32-byte entity descriptor from ROM 0x03F2FA for even type_id in [0x0002, 0x00C6].
[[nodiscard]] std::optional<RomEntityDescriptor> load_rom_entity_descriptor(
    std::span<const std::uint8_t> rom,
    std::uint16_t type_id) noexcept;

// Applies ROM descriptor geometry and flag fields onto a NativeEntity slot (models 0x008D06).
void apply_descriptor_to_entity(const RomEntityDescriptor& desc, NativeEntity& ent) noexcept;

// Looks up 32-bit update handler PC from ROM 0x00DF54 for even type_id in [0x0016, 0x00C6].
[[nodiscard]] std::optional<std::uint32_t> lookup_entity_update_handler(
    std::span<const std::uint8_t> rom,
    std::uint16_t type_id) noexcept;

} // namespace oasis::game::entity
