#include "game/world/town_scene_loader.hpp"

#include "core/sha256.hpp"
#include "game/graphics_decompress.hpp"
#include "game/resource_loader.hpp"

#include <algorithm>
#include <stdexcept>

namespace oasis::game::world {

namespace {

constexpr std::uint32_t kTownScriptObjBankPtr1 = 0x0464F0U; // -> 0x2AF51E
constexpr std::uint32_t kTownScriptObjBankPtr2 = 0x0464F8U; // -> 0x163BDA
constexpr std::uint32_t kTownChestSpawnXAddr = 0x046512U;
constexpr std::uint32_t kTownEnemySpawnListAddr = 0x046522U;
constexpr std::size_t kTownEnemyRecordStride = 18U;
constexpr std::size_t kTownMaxSlots = 10U; // Slots 5..14 active in Screen(1,3)

std::uint16_t read_be16(std::span<const std::uint8_t> data, std::size_t off) {
    if (off + 2U > data.size()) {
        throw std::out_of_range("town_scene_loader read_be16 out of range");
    }
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[off]) << 8U) |
                                      data[off + 1U]);
}

std::uint32_t read_be32(std::span<const std::uint8_t> data, std::size_t off) {
    if (off + 4U > data.size()) {
        throw std::out_of_range("town_scene_loader read_be32 out of range");
    }
    return (static_cast<std::uint32_t>(data[off]) << 24U) |
           (static_cast<std::uint32_t>(data[off + 1U]) << 16U) |
           (static_cast<std::uint32_t>(data[off + 2U]) << 8U) |
           static_cast<std::uint32_t>(data[off + 3U]);
}

TownStreamChunk decompress_rom_stream(std::span<const std::uint8_t> rom, std::uint32_t rom_start) {
    if (rom_start >= rom.size()) {
        throw std::out_of_range("town stream rom_start out of range");
    }
    std::vector<std::uint8_t> buffer(64U * 1024U);
    const auto res = decompress_graphics(rom.subspan(rom_start), buffer);
    buffer.resize(res.output_size);

    TownStreamChunk chunk{};
    chunk.rom_start = rom_start;
    chunk.compressed_size = res.source_consumed;
    chunk.decoded_size = res.output_size;
    chunk.decoded_sha256 = core::compute_sha256(buffer);
    chunk.decoded_bytes = std::move(buffer);
    return chunk;
}

} // namespace

LoadedTownScene load_town_scene_from_rom(std::span<const std::uint8_t> rom) {
    LoadedTownScene scene{};
    scene.descriptor = load_screen_descriptor(rom, ScreenId{1U, 3U});
    const auto rids = scene.descriptor.resource_ids();

    // 1..4: ScreenDescriptor resource IDs 13, 14, 15, 16 resolved via 0x05CE96
    for (std::size_t i = 0; i < 4U; ++i) {
        const std::uint32_t ptr = read_be32(
            rom, kResourceTableAddress + static_cast<std::size_t>(rids[i]) * 4U);
        scene.stream_chunks[i] = decompress_rom_stream(rom, ptr);
    }

    // 5..6: Town Screen(1,3) object/sprite tile banks referenced from 0x0464EE / 0x0464F6
    scene.stream_chunks[4] = decompress_rom_stream(rom, read_be32(rom, kTownScriptObjBankPtr1));
    scene.stream_chunks[5] = decompress_rom_stream(rom, read_be32(rom, kTownScriptObjBankPtr2));

    // 7: Primary Town Screen(1,3) map layout stream (0x213850)
    scene.stream_chunks[6] = decompress_rom_stream(rom, scene.descriptor.primary_stream_pointer());

    // Entity 0 in Town scene vector: Town gate/chest 0x0060 at (160, 120) from 0x046512
    NativeEntity gate_obj{};
    gate_obj.type_id = 0x0060U;
    gate_obj.world_x = static_cast<std::int32_t>(read_be16(rom, kTownChestSpawnXAddr));
    gate_obj.world_y = static_cast<std::int32_t>(read_be16(rom, kTownChestSpawnXAddr + 2U));
    gate_obj.x_fp = gate_obj.world_x * 0x10000;
    gate_obj.y_fp = gate_obj.world_y * 0x10000;
    gate_obj.tile_base = 0x0490U;
    gate_obj.active = true;
    scene.entities.push_back(gate_obj);

    // Next 10 Town entities: Enemy 0x002C guards from ROM 0x046522.. (18-byte records)
    for (std::size_t idx = 0; idx < kTownMaxSlots; ++idx) {
        const std::size_t rec = kTownEnemySpawnListAddr + idx * kTownEnemyRecordStride;
        const std::uint16_t marker = read_be16(rom, rec);
        if (marker == 0U) {
            break;
        }
        NativeEntity guard{};
        guard.type_id = read_be16(rom, rec + 2U);
        guard.facing = read_be16(rom, rec + 4U);
        guard.world_x = static_cast<std::int32_t>(read_be16(rom, rec + 6U));
        guard.world_y = static_cast<std::int32_t>(read_be16(rom, rec + 8U));
        guard.x_fp = guard.world_x * 0x10000;
        guard.y_fp = guard.world_y * 0x10000;
        const std::int16_t hp_val = static_cast<std::int16_t>(read_be16(rom, rec + 10U));
        guard.hp = hp_val;
        guard.max_hp = hp_val;
        guard.defense = 0;
        guard.half_width = 10;
        guard.half_depth = 7;
        guard.height = 16;
        guard.tile_base = 0x0200U;
        guard.active = true;
        scene.entities.push_back(guard);
    }

    return scene;
}

void upload_town_tiles_to_vdp(const LoadedTownScene& scene, genesis::VdpState& vdp) {
    // Load decoded Town 4bpp pattern payloads into VRAM sequentially for tileset & sprites.
    std::size_t cursor = 0x4000U;
    for (std::size_t i = 0; i < 6U; ++i) {
        const auto& bytes = scene.stream_chunks[i].decoded_bytes;
        const std::size_t copy_len = std::min(bytes.size(), vdp.vram.size() - cursor);
        if (copy_len > 0U) {
            std::copy_n(bytes.begin(), copy_len, vdp.vram.begin() + static_cast<std::ptrdiff_t>(cursor));
            cursor += (copy_len + 31U) & ~std::size_t{31U};
        }
        if (cursor >= vdp.vram.size()) {
            break;
        }
    }
}

} // namespace oasis::game::world
