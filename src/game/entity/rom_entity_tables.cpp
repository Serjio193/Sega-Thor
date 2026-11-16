#include "game/entity/rom_entity_tables.hpp"

namespace oasis::game::entity {

namespace {

[[nodiscard]] std::uint16_t read_be16(std::span<const std::uint8_t> rom, std::size_t off) noexcept {
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(rom[off]) << 8U) | rom[off + 1U]);
}

[[nodiscard]] std::uint32_t read_be32(std::span<const std::uint8_t> rom, std::size_t off) noexcept {
    return (static_cast<std::uint32_t>(rom[off]) << 24U) |
           (static_cast<std::uint32_t>(rom[off + 1U]) << 16U) |
           (static_cast<std::uint32_t>(rom[off + 2U]) << 8U) |
           static_cast<std::uint32_t>(rom[off + 3U]);
}

} // namespace

std::optional<RomEntityDescriptor> load_rom_entity_descriptor(
    std::span<const std::uint8_t> rom,
    std::uint16_t type_id) noexcept {
    if ((type_id & 1U) != 0U || type_id < kFirstDescriptorType || type_id > kLastDescriptorType) {
        return std::nullopt;
    }

    // ROM 0x008D06: addr = 0x03F2FA - 0x20 + (type_id << 4)
    const std::size_t addr =
        static_cast<std::size_t>(kEntityDescriptorTableRom) - 0x20U + (static_cast<std::size_t>(type_id) << 4U);
    if (addr + kEntityDescriptorStride > rom.size()) {
        return std::nullopt;
    }

    RomEntityDescriptor d{};
    d.type_id = type_id;
    d.descriptor_addr = static_cast<std::uint32_t>(addr);
    d.z_radius_4a = read_be16(rom, addr + 0x00U);
    d.xy_radius_46 = rom[addr + 0x02U];
    d.dim_x_48 = rom[addr + 0x03U];
    d.dim_y_49 = rom[addr + 0x04U];
    d.render_prio_3d = rom[addr + 0x05U];
    d.mass_4c = rom[addr + 0x06U];
    d.flags_3a = rom[addr + 0x07U];
    d.flags_36 = rom[addr + 0x08U];
    d.flags_37 = rom[addr + 0x09U];
    d.flags_38 = rom[addr + 0x0AU];
    d.flags_3c = rom[addr + 0x0BU];
    d.anim_ptr_1a = read_be32(rom, addr + 0x0CU);
    d.anim_ptr_1e = read_be32(rom, addr + 0x10U);
    d.anim_ptr_26 = read_be32(rom, addr + 0x14U);
    d.anim_ptr_22 = read_be32(rom, addr + 0x18U);
    d.ai_ptr_5e = read_be32(rom, addr + 0x1CU);
    return d;
}

void apply_descriptor_to_entity(const RomEntityDescriptor& desc, NativeEntity& ent) noexcept {
    ent.type_id = desc.type_id;
    ent.height = static_cast<std::int16_t>(desc.z_radius_4a);
    ent.half_width = static_cast<std::int16_t>(desc.xy_radius_46);
    ent.half_depth = static_cast<std::int16_t>(desc.xy_radius_46);
    ent.base_half_w = desc.dim_x_48;
    ent.base_half_d = desc.dim_y_49;
    ent.shadow_idx = desc.render_prio_3d;
    ent.flags3a = desc.flags_3a;
    ent.flags36 = desc.flags_36;
    ent.flags37 = desc.flags_37;
    ent.flags38 = desc.flags_38;
}

std::optional<std::uint32_t> lookup_entity_update_handler(
    std::span<const std::uint8_t> rom,
    std::uint16_t type_id) noexcept {
    if ((type_id & 1U) != 0U || type_id < kFirstUpdateType || type_id > kLastUpdateType) {
        return std::nullopt;
    }

    // ROM 0x00DF54: addr = 0x00DF54 - 0x2C + type_id * 2
    const std::size_t addr =
        static_cast<std::size_t>(kEntityUpdateTableRom) - 0x2CU + static_cast<std::size_t>(type_id) * 2U;
    if (addr + 4U > rom.size()) {
        return std::nullopt;
    }
    return read_be32(rom, addr);
}

} // namespace oasis::game::entity
