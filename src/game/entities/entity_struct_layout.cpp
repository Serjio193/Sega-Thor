#include "game/entities/entity_struct_layout.hpp"

namespace oasis::game::entities {

namespace {

[[nodiscard]] constexpr std::uint16_t read_be16(
    std::span<const std::uint8_t> data,
    std::size_t offset) noexcept {
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(data[offset]) << 8U) |
        static_cast<std::uint16_t>(data[offset + 1U]));
}

[[nodiscard]] constexpr std::uint32_t read_be32(
    std::span<const std::uint8_t> data,
    std::size_t offset) noexcept {
    return (static_cast<std::uint32_t>(data[offset]) << 24U) |
           (static_cast<std::uint32_t>(data[offset + 1U]) << 16U) |
           (static_cast<std::uint32_t>(data[offset + 2U]) << 8U) |
           static_cast<std::uint32_t>(data[offset + 3U]);
}

void apply_descriptor_fields(
    const EntityDescriptor32& desc,
    EntityConstructorState& state) noexcept {
    state.entity_id = desc.entity_id;
    state.height_z = desc.height_z;
    state.radius_extent = desc.radius_extent;
    state.radius_42 = desc.radius_extent;
    state.radius_44 = desc.radius_extent;
    state.hitbox_half_w = desc.hitbox_half_w;
    state.hitbox_half_h = desc.hitbox_half_h;
    state.weight_flags = desc.weight_flags;
    state.shadow_z_offset = desc.shadow_z_offset;
    state.trait_flags = desc.trait_flags;
    state.collision_flags = desc.collision_flags;
    state.combat_flags = desc.combat_flags;
    state.render_flags = desc.render_flags;
    state.behavior_flags = desc.behavior_flags;
    state.vram_tile_ptr = desc.vram_tile_ptr;
    state.sprite_frame_ptr = desc.sprite_frame_ptr;
    state.anim_script_ptr = desc.anim_script_ptr;
    state.hitbox_table_ptr = desc.hitbox_table_ptr;
    state.secondary_ai_ptr = state.is_lightweight_slot ? 0U : desc.secondary_ai_ptr;
}

} // namespace

std::optional<EntityDescriptor32> decode_entity_descriptor32(
    std::span<const std::uint8_t> rom,
    std::uint16_t entity_id) noexcept {
    if (entity_id < 2U || (entity_id & 1U) != 0U) {
        return std::nullopt;
    }

    const std::uint32_t addr =
        kEntityMasterDescriptorTableRom + static_cast<std::uint32_t>(entity_id - 2U) * 16U;
    if (static_cast<std::size_t>(addr) + kEntityDescriptorStrideBytes > rom.size()) {
        return std::nullopt;
    }

    const auto entry = rom.subspan(addr, kEntityDescriptorStrideBytes);
    EntityDescriptor32 desc{};
    desc.entity_id = entity_id;
    desc.rom_addr = addr;
    desc.height_z = read_be16(entry, 0x00U);
    desc.radius_extent = entry[0x02U];
    desc.hitbox_half_w = entry[0x03U];
    desc.hitbox_half_h = entry[0x04U];
    desc.weight_flags = entry[0x05U];
    desc.shadow_z_offset = entry[0x06U];
    desc.trait_flags = entry[0x07U];
    desc.collision_flags = entry[0x08U];
    desc.combat_flags = entry[0x09U];
    desc.render_flags = entry[0x0AU];
    desc.behavior_flags = entry[0x0BU];
    desc.vram_tile_ptr = read_be32(entry, 0x0CU);
    desc.sprite_frame_ptr = read_be32(entry, 0x10U);
    desc.anim_script_ptr = read_be32(entry, 0x14U);
    desc.hitbox_table_ptr = read_be32(entry, 0x18U);
    desc.secondary_ai_ptr = read_be32(entry, 0x1CU);
    return desc;
}

std::optional<EntityConstructorState> init_entity_from_descriptor(
    std::span<const std::uint8_t> rom,
    std::uint16_t entity_id,
    bool is_lightweight_slot) noexcept {
    const auto desc = decode_entity_descriptor32(rom, entity_id);
    if (!desc.has_value()) {
        return std::nullopt;
    }

    EntityConstructorState state{};
    state.is_lightweight_slot = is_lightweight_slot;
    state.cleared_words = is_lightweight_slot ? kLightEntityClearWords : kFullEntityClearWords;
    state.timer_5c = 0xFFFFU;
    state.timer_2c = 0xFFFFU;
    state.timer_34 = 0xFFFFU;
    apply_descriptor_fields(*desc, state);
    return state;
}

bool reinit_entity_pointers(
    std::span<const std::uint8_t> rom,
    std::uint16_t entity_id,
    EntityConstructorState& state) noexcept {
    const auto desc = decode_entity_descriptor32(rom, entity_id);
    if (!desc.has_value()) {
        return false;
    }
    apply_descriptor_fields(*desc, state);
    return true;
}

} // namespace oasis::game::entities
