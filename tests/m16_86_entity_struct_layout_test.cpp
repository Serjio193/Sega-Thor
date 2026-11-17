#include "core/rom.hpp"
#include "game/entities/entity_struct_layout.hpp"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace {

using namespace oasis::game::entities;

void write_be16(std::vector<std::uint8_t>& buf, std::size_t off, std::uint16_t val) {
    buf[off] = static_cast<std::uint8_t>((val >> 8U) & 0xFFU);
    buf[off + 1U] = static_cast<std::uint8_t>(val & 0xFFU);
}

void write_be32(std::vector<std::uint8_t>& buf, std::size_t off, std::uint32_t val) {
    buf[off] = static_cast<std::uint8_t>((val >> 24U) & 0xFFU);
    buf[off + 1U] = static_cast<std::uint8_t>((val >> 16U) & 0xFFU);
    buf[off + 2U] = static_cast<std::uint8_t>((val >> 8U) & 0xFFU);
    buf[off + 3U] = static_cast<std::uint8_t>(val & 0xFFU);
}

void test_synthetic_descriptor_and_constructor() {
    std::vector<std::uint8_t> rom(0x040000U, 0x00U);

    // Populate entity_id = 4 at 0x03F2FA + (4 - 2) * 16 = 0x03F31A
    const std::size_t base = kEntityMasterDescriptorTableRom + 2U * 16U;
    write_be16(rom, base + 0x00U, 0x0030U); // height_z = 48
    rom[base + 0x02U] = 0x0CU;             // radius_extent = 12
    rom[base + 0x03U] = 0x0EU;             // hitbox_half_w = 14
    rom[base + 0x04U] = 0x10U;             // hitbox_half_h = 16
    rom[base + 0x05U] = 0x05U;             // weight_flags
    rom[base + 0x06U] = 0x02U;             // shadow_z_offset
    rom[base + 0x07U] = 0x01U;             // trait_flags (bit0 = spirit)
    rom[base + 0x08U] = 0x11U;             // collision_flags
    rom[base + 0x09U] = 0x22U;             // combat_flags
    rom[base + 0x0AU] = 0x33U;             // render_flags
    rom[base + 0x0BU] = 0x44U;             // behavior_flags
    write_be32(rom, base + 0x0CU, 0x0013CDFCU);
    write_be32(rom, base + 0x10U, 0x000C0000U);
    write_be32(rom, base + 0x14U, 0x0003FF7AU);
    write_be32(rom, base + 0x18U, 0x00058000U);
    write_be32(rom, base + 0x1CU, 0x0000A1B2U);

    // Invalid IDs rejected
    assert(!decode_entity_descriptor32(rom, 0).has_value());
    assert(!decode_entity_descriptor32(rom, 3).has_value());

    const auto desc = decode_entity_descriptor32(rom, 4);
    assert(desc.has_value());
    assert(desc->rom_addr == base);
    assert(desc->height_z == 48U);
    assert(desc->radius_extent == 0x0CU);
    assert(desc->hitbox_half_w == 0x0EU);
    assert(desc->hitbox_half_h == 0x10U);
    assert(desc->weight_flags == 0x05U);
    assert(desc->shadow_z_offset == 0x02U);
    assert(desc->trait_flags == 0x01U);
    assert(desc->collision_flags == 0x11U);
    assert(desc->combat_flags == 0x22U);
    assert(desc->render_flags == 0x33U);
    assert(desc->behavior_flags == 0x44U);
    assert(desc->vram_tile_ptr == 0x0013CDFCU);
    assert(desc->sprite_frame_ptr == 0x000C0000U);
    assert(desc->anim_script_ptr == 0x0003FF7AU);
    assert(desc->hitbox_table_ptr == 0x00058000U);
    assert(desc->secondary_ai_ptr == 0x0000A1B2U);

    // Full slot (< 0xFF2954): clears 0x5D words, keeps secondary_ai_ptr
    assert(!is_lightweight_entity_ram_addr(0xFF2800U));
    const auto full = init_entity_from_descriptor(rom, 4, false);
    assert(full.has_value());
    assert(full->cleared_words == 0x5DU);
    assert(full->timer_5c == 0xFFFFU);
    assert(full->timer_2c == 0xFFFFU);
    assert(full->timer_34 == 0xFFFFU);
    assert(full->radius_42 == 0x0CU && full->radius_44 == 0x0CU);
    assert(full->secondary_ai_ptr == 0x0000A1B2U);

    // Lightweight slot (>= 0xFF2954): clears 0x2C words, suppresses secondary_ai_ptr
    assert(is_lightweight_entity_ram_addr(0xFF2954U));
    auto light = init_entity_from_descriptor(rom, 4, true);
    assert(light.has_value());
    assert(light->cleared_words == 0x2CU);
    assert(light->secondary_ai_ptr == 0U);

    // reinit_entity_pointers (0x008DA4) preserves timers while updating descriptor fields
    light->timer_5c = 0x0010U;
    light->timer_2c = 0x0020U;
    light->timer_34 = 0x0030U;
    const std::size_t base2 = kEntityMasterDescriptorTableRom;
    write_be32(rom, base2 + 0x10U, 0x000D1234U);
    write_be32(rom, base2 + 0x1CU, 0x0000BEEFU);
    assert(reinit_entity_pointers(rom, 2, *light));
    assert(light->entity_id == 2U);
    assert(light->sprite_frame_ptr == 0x000D1234U);
    assert(light->secondary_ai_ptr == 0U); // still lightweight
    assert(light->timer_5c == 0x0010U);
    assert(light->timer_2c == 0x0020U);
    assert(light->timer_34 == 0x0030U);
}

void test_real_rom_ali_id2_if_available() {
    const std::filesystem::path rom_path = "local-roms/Beyond Oasis (USA).md";
    if (!std::filesystem::exists(rom_path)) {
        return;
    }
    const auto rom = oasis::Rom::load(rom_path);
    const auto ali = init_entity_from_descriptor(rom.bytes(), 2, false);
    assert(ali.has_value());
    assert(ali->height_z == 48U);
    assert(ali->radius_extent == 10U);
    assert(ali->hitbox_half_w == 10U);
    assert(ali->hitbox_half_h == 10U);
    assert(ali->vram_tile_ptr == 0x13CDFCU);
    assert(ali->sprite_frame_ptr == 0x0C0000U);
    assert(ali->anim_script_ptr == 0x03FF7AU);
    assert(ali->hitbox_table_ptr == 0x058000U);
}

} // namespace

int main() {
    test_synthetic_descriptor_and_constructor();
    test_real_rom_ali_id2_if_available();
    return 0;
}
