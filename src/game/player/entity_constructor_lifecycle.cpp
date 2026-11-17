#include "game/player/entity_constructor_lifecycle.hpp"

#include <algorithm>

namespace oasis::game::player {

namespace {

inline void write_u16_be(std::span<std::uint8_t> buf, std::size_t offset, std::uint16_t val) noexcept {
    if (offset + 2U <= buf.size()) {
        buf[offset] = static_cast<std::uint8_t>((val >> 8U) & 0xFFU);
        buf[offset + 1U] = static_cast<std::uint8_t>(val & 0xFFU);
    }
}

inline void write_u32_be(std::span<std::uint8_t> buf, std::size_t offset, std::uint32_t val) noexcept {
    if (offset + 4U <= buf.size()) {
        buf[offset] = static_cast<std::uint8_t>((val >> 24U) & 0xFFU);
        buf[offset + 1U] = static_cast<std::uint8_t>((val >> 16U) & 0xFFU);
        buf[offset + 2U] = static_cast<std::uint8_t>((val >> 8U) & 0xFFU);
        buf[offset + 3U] = static_cast<std::uint8_t>(val & 0xFFU);
    }
}

inline std::uint16_t read_u16_be(std::span<const std::uint8_t> buf, std::size_t offset) noexcept {
    if (offset + 2U <= buf.size()) {
        return static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(buf[offset]) << 8U) |
            static_cast<std::uint16_t>(buf[offset + 1U]));
    }
    return 0U;
}

inline std::uint32_t read_u32_be(std::span<const std::uint8_t> buf, std::size_t offset) noexcept {
    if (offset + 4U <= buf.size()) {
        return (static_cast<std::uint32_t>(buf[offset]) << 24U) |
               (static_cast<std::uint32_t>(buf[offset + 1U]) << 16U) |
               (static_cast<std::uint32_t>(buf[offset + 2U]) << 8U) |
               static_cast<std::uint32_t>(buf[offset + 3U]);
    }
    return 0U;
}

} // namespace

void clear_entity_slot_90(std::span<std::uint8_t> slot) noexcept {
    if (slot.size() < 90U) {
        return;
    }
    write_u16_be(slot, 0x00U, 0U);
    write_u16_be(slot, 0x04U, 0U);
    write_u16_be(slot, 0x06U, 0U);
    write_u16_be(slot, 0x2AU, 0U);
    write_u16_be(slot, 0x2CU, 0xFFFFU);
    write_u16_be(slot, 0x34U, 0xFFFFU);
}

void clear_entity_slot_188(std::span<std::uint8_t> slot) noexcept {
    if (slot.size() < 188U) {
        return;
    }
    write_u16_be(slot, 0x00U, 0U);
    write_u16_be(slot, 0x04U, 0U);
    write_u16_be(slot, 0x06U, 0U);
    write_u16_be(slot, 0x2AU, 0U);
    write_u16_be(slot, 0x2CU, 0xFFFFU);
    write_u16_be(slot, 0x34U, 0xFFFFU);
    write_u16_be(slot, 0x5CU, 0xFFFFU);
}

void clear_special_slot_8(std::span<std::uint8_t> slot) noexcept {
    if (slot.size() < 8U) {
        return;
    }
    write_u16_be(slot, 0x00U, 0U);
}

void EntityPoolSet::init_default() noexcept {
    pool1_90b.assign(EntityLifecycleConfig::kPool1Slots, {});
    pool2_90b.assign(EntityLifecycleConfig::kPool2Slots, {});
    pool3_90b.assign(EntityLifecycleConfig::kPool3Slots, {});
    pool4_8b.assign(EntityLifecycleConfig::kPool4Slots, {});
    player_slot.fill(0U);
    spirit_slot.fill(0U);
    pool5_188b.assign(EntityLifecycleConfig::kPool5Slots, {});
}

void EntityPoolSet::clear_subsystems() noexcept {
    for (auto& slot : pool1_90b) {
        clear_entity_slot_90(slot);
    }
    for (auto& slot : pool2_90b) {
        clear_entity_slot_90(slot);
    }
    for (auto& slot : pool3_90b) {
        clear_entity_slot_90(slot);
    }
    for (auto& slot : pool4_8b) {
        clear_special_slot_8(slot);
    }
    for (auto& slot : pool5_188b) {
        clear_entity_slot_188(slot);
    }
    // player_slot and spirit_slot are preserved!
}

std::optional<EntityDescriptor> read_entity_descriptor(
    std::span<const std::uint8_t> rom_or_table,
    std::uint16_t entity_type,
    bool is_full_rom) noexcept {
    if (entity_type < 2U) {
        return std::nullopt;
    }
    const std::size_t base = is_full_rom
        ? static_cast<std::size_t>(EntityLifecycleConfig::kDescriptorTableRomAddr)
        : 0U;
    const std::size_t offset = base + static_cast<std::size_t>(entity_type - 2U) * 16U;
    if (offset + EntityLifecycleConfig::kDescriptorStrideBytes > rom_or_table.size()) {
        return std::nullopt;
    }

    EntityDescriptor desc{};
    desc.sprite_id = read_u16_be(rom_or_table, offset + 0U);
    desc.radius = rom_or_table[offset + 2U];
    desc.offset_x = static_cast<std::int8_t>(rom_or_table[offset + 3U]);
    desc.offset_y = static_cast<std::int8_t>(rom_or_table[offset + 4U]);
    desc.tile_attribute = rom_or_table[offset + 5U];
    desc.height = rom_or_table[offset + 6U];
    desc.anim_flags = rom_or_table[offset + 7U];
    desc.collision_flags = rom_or_table[offset + 8U];
    desc.defense_flags = rom_or_table[offset + 9U];
    desc.render_flags = rom_or_table[offset + 10U];
    desc.ai_flags = rom_or_table[offset + 11U];
    desc.fn_ptr_1a = read_u32_be(rom_or_table, offset + 12U);
    desc.fn_ptr_1e = read_u32_be(rom_or_table, offset + 16U);
    desc.fn_ptr_26 = read_u32_be(rom_or_table, offset + 20U);
    desc.fn_ptr_22 = read_u32_be(rom_or_table, offset + 24U);
    desc.fn_ptr_5e = read_u32_be(rom_or_table, offset + 28U);
    return desc;
}

void construct_entity(
    std::span<std::uint8_t> slot,
    std::uint16_t entity_type,
    const EntityDescriptor& desc,
    bool is_small_slot) noexcept {
    const std::size_t slot_len = is_small_slot ? 90U : 188U;
    if (slot.size() < slot_len) {
        return;
    }

    // 0x008D0A..0x008D22: Zero clear words starting at offset 2
    std::fill(slot.begin() + 2U, slot.begin() + slot_len, static_cast<std::uint8_t>(0U));

    // Store type at +0x00
    write_u16_be(slot, 0x00U, entity_type);

    // 0x008D38..0x008D6E: Populate descriptor fields
    write_u16_be(slot, 0x4AU, desc.sprite_id);
    write_u16_be(slot, 0x46U, desc.radius);
    write_u16_be(slot, 0x42U, desc.radius);
    write_u16_be(slot, 0x44U, desc.radius);
    slot[0x48U] = static_cast<std::uint8_t>(desc.offset_x);
    slot[0x49U] = static_cast<std::uint8_t>(desc.offset_y);
    slot[0x3DU] = desc.tile_attribute;
    write_u16_be(slot, 0x4CU, desc.height);
    slot[0x3AU] = desc.anim_flags;
    slot[0x36U] = desc.collision_flags;
    slot[0x37U] = desc.defense_flags;
    slot[0x38U] = desc.render_flags;
    slot[0x3CU] = desc.ai_flags;

    // 0x008D70..0x008D7E: Script function pointers
    write_u32_be(slot, 0x1AU, desc.fn_ptr_1a);
    write_u32_be(slot, 0x1EU, desc.fn_ptr_1e);
    write_u32_be(slot, 0x26U, desc.fn_ptr_26);
    write_u32_be(slot, 0x22U, desc.fn_ptr_22);

    if (!is_small_slot) {
        // 0x008D88..0x008D8E: Large entity only (+0x5E pointer, +0x5C timer)
        write_u32_be(slot, 0x5EU, desc.fn_ptr_5e);
        write_u16_be(slot, 0x5CU, 0xFFFFU);
    }

    // 0x008D92..0x008D9A: Initial timers for both small and large entities
    write_u16_be(slot, 0x2CU, 0xFFFFU);
    write_u16_be(slot, 0x34U, 0xFFFFU);
}

void reinit_entity_script_pointers(
    std::span<std::uint8_t> slot,
    const EntityDescriptor& desc) noexcept {
    if (slot.size() < 90U) {
        return;
    }
    write_u32_be(slot, 0x1AU, desc.fn_ptr_1a);
    write_u32_be(slot, 0x1EU, desc.fn_ptr_1e);
    write_u32_be(slot, 0x26U, desc.fn_ptr_26);
    write_u32_be(slot, 0x22U, desc.fn_ptr_22);
    if (slot.size() >= 188U) {
        write_u32_be(slot, 0x5EU, desc.fn_ptr_5e);
    }
}

std::uint8_t step_spirit_glow_pulse(
    std::uint8_t current_glow,
    bool has_spirit,
    std::uint16_t anim_timer) noexcept {
    std::uint8_t glow = current_glow;
    if (has_spirit) {
        if (glow < 3U) {
            return static_cast<std::uint8_t>(glow + 1U);
        }
        glow = 3U;
        if (anim_timer >= 20U && (anim_timer & 1U) == 0U) {
            return static_cast<std::uint8_t>(glow + 1U); // pulses to 4
        }
        return glow;
    } else {
        if (glow == 0U) {
            return 0U;
        }
        if (glow >= 4U) {
            return 2U;
        }
        return static_cast<std::uint8_t>(glow - 1U);
    }
}

} // namespace oasis::game::player
