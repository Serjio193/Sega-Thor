#include "game/screen_instance.hpp"

#include "game/graphics_decompress.hpp"
#include "game/resource_loader.hpp"

#include <algorithm>
#include <stdexcept>

namespace oasis::game {

ScreenInstance ScreenInstance::load_from_rom(
    std::span<const std::uint8_t> rom,
    world::ScreenId id) {
    ScreenInstance inst{};
    inst.screen_id = id;
    inst.setup_id = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(id.group) << 8U) | id.index);

    const auto desc = world::load_screen_descriptor(rom, id);
    inst.descriptor_address = desc.rom_address;
    inst.primary_stream_pointer = desc.primary_stream_pointer();
    inst.gfx_resource_ids = desc.resource_ids();

    inst.primary_bytes.resize(64U * 1024U);
    const auto decomp_res = decompress_graphics(
        rom.subspan(inst.primary_stream_pointer), inst.primary_bytes);
    inst.primary_bytes.resize(decomp_res.output_size);

    inst.rebind_views();

    inst.tilemap.clear();
    if (inst.primary_bytes.size() >= 0x2004U) {
        inst.tilemap.reserve(1792U);
        for (std::size_t i = 0; i < 1792U; ++i) {
            const auto offset = 0x1204U + i * 2U;
            const auto w = static_cast<std::uint16_t>(
                (static_cast<std::uint16_t>(inst.primary_bytes[offset]) << 8U) |
                static_cast<std::uint16_t>(inst.primary_bytes[offset + 1U]));
            inst.tilemap.push_back(w);
        }
    }

    inst.tiles.clear();
    for (const auto rid : inst.gfx_resource_ids) {
        const auto loaded = load_graphics_resource(rom, rid);
        const auto count = loaded.bytes.size() / 32U;
        for (std::size_t i = 0; i < count; ++i) {
            inst.tiles.push_back(decode_genesis_4bpp_tile(
                std::span<const std::uint8_t>(loaded.bytes).subspan(i * 32U, 32U)));
        }
    }

    if (inst.primary_bytes.size() >= 0x3044U) {
        inst.palettes[1] = decode_genesis_palette16(
            std::span<const std::uint8_t>(inst.primary_bytes).subspan(0x3004U, 32U));
        inst.palettes[2] = decode_genesis_palette16(
            std::span<const std::uint8_t>(inst.primary_bytes).subspan(0x3024U, 32U));
    }
    inst.palettes[0] = inst.palettes[1];
    inst.palettes[3] = inst.palettes[2];

    inst.entities.clear();
    if (id.group == 1 && id.index == 0) {
        inst.entities.push_back(
            entities::ScreenEntity::spawn_from_rom(rom, 0x002A, 192, 160));
        inst.entities.push_back(
            entities::ScreenEntity::spawn_from_rom(rom, 0x0048, 160, 120, 0, 2));
    } else if (id.group == 1 && id.index == 1) {
        inst.entities.push_back(
            entities::ScreenEntity::spawn_from_rom(rom, 0x002A, 224, 144));
        inst.entities.push_back(
            entities::ScreenEntity::spawn_from_rom(rom, 0x002A, 160, 200));
        auto flame = entities::ScreenEntity::spawn_from_rom(rom, 0x004C, 192, 160);
        flame.set_persistent_id(1);
        inst.entities.push_back(std::move(flame));
    }

    return inst;
}

void ScreenInstance::rebind_views() noexcept {
    const auto grid = world::ByteGridView::from_parent_output(
        primary_bytes, 0x204U, 64, 64, 64, 6);
    if (grid) {
        terrain = *grid;
    }
}

} // namespace oasis::game
