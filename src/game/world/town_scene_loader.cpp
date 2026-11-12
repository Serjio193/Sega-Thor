#include "game/world/town_scene_loader.hpp"

#include "core/sha256.hpp"
#include "game/graphics_decompress.hpp"
#include "game/resource_loader.hpp"

#include <algorithm>
#include <stdexcept>

namespace oasis::game::world {

namespace {

constexpr std::uint32_t kTownScriptObjBankPtr1 = 0x0464F0U; // -> 0x2AF51E (0x002C @ VRAM 0x4000)
constexpr std::uint32_t kTownScriptObjBankPtr2 = 0x0464F8U; // -> 0x163BDA (0x0060 @ VRAM 0x9200)
constexpr std::uint32_t kTownChestSpawnXAddr = 0x046512U;
constexpr std::uint32_t kTownEnemySpawnListAddr = 0x046522U;
constexpr std::size_t kTownEnemyRecordStride = 18U;
constexpr std::size_t kTownEnemySpawnCount = 14U; // ROM 0x046522..0x04661D: 14 active 0x002C records (Slots 5..18)

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

NativeEntity make_guard_from_rom(std::span<const std::uint8_t> rom, std::size_t rec,
                                 std::uint16_t slot_index) {
    // ROM 0x025568/0x025570 variant tables indexed by variant_96 & 3:
    static constexpr std::uint16_t kDefenseByVariant[4] = {0x0000U, 0x0004U, 0x0006U, 0x0000U};
    static constexpr std::uint16_t kPrngMaskByVariant[4] = {0x0703U, 0x0000U, 0x0000U, 0x0000U};
    static constexpr std::uint16_t kVariant96Table[4] = {0x0001U, 0x0001U, 0x0000U, 0x0000U};

    NativeEntity guard{};
    guard.slot_index = slot_index;
    guard.type_id = read_be16(rom, rec + 2U);
    const std::uint16_t raw_var = read_be16(rom, rec + 4U);
    guard.world_x = static_cast<std::int32_t>(read_be16(rom, rec + 6U));
    guard.world_y = static_cast<std::int32_t>(read_be16(rom, rec + 8U));
    guard.x_fp = guard.world_x * 0x10000;
    guard.y_fp = guard.world_y * 0x10000;

    const std::int16_t hp_val = static_cast<std::int16_t>(read_be16(rom, rec + 10U));
    guard.hp = hp_val;
    guard.max_hp = hp_val;
    const std::uint16_t spawn_flags = read_be16(rom, rec + 12U);
    const std::uint8_t sub_mode = static_cast<std::uint8_t>(read_be16(rom, rec + 14U) >> 8U);
    guard.init_sub = sub_mode;
    guard.tile_base = read_be16(rom, rec + 16U); // 0x2200 (pal 1) or 0x0200 (pal 0)

    // Populate authentic ROM 0x03F59A defaults for Entity 0x002C
    guard.height = 16;
    guard.base_half_w = 10;
    guard.base_half_d = 10;
    guard.half_width = 10;
    guard.half_depth = 15;
    guard.shadow_idx = 0x10U;
    guard.flags38 = 0x10U;
    guard.active = true;

    guard.variant_96 = raw_var;
    if ((spawn_flags & 0x8000U) != 0U) {
        guard.flags38 |= 0x20U;
    }
    const std::uint16_t vidx = raw_var & 3U;
    (void)kVariant96Table;
    guard.defense = static_cast<std::int16_t>(0x0080U + kDefenseByVariant[vidx]);
    guard.prng_mask_a0 = (vidx == 0U) ? kPrngMaskByVariant[0] : 0x0307U;

    if (sub_mode == 4U) {
        // ROM 0x025578: Rooftop spawn (z=256, state=0x14, anim=0x12, hidden until dist <= 48)
        guard.state = 0x14U;
        guard.anim_frame = 0x12U;
        guard.world_z = 256;
        guard.z_fp = 256 * 0x10000;
        guard.flags36 |= 0x02U;
        guard.flags37 |= 0x01U;
        guard.flags38 |= 0x0CU;
        guard.overlay_3e = 0U;
        guard.frame_dir = 0xFFFFU;
    } else {
        guard.facing = 2U;
        guard.state = 0x00U;
        guard.anim_frame = 0x00U;
        guard.frame_id = 0U;
        guard.frame_dir = 2U;
        guard.flags37 = 0x24U;
        guard.overlay_3e = 0xFFFFU;
        guard.ai_timer = 10U;
    }
    return guard;
}

} // namespace

LoadedTownScene load_world_screen_from_rom(std::span<const std::uint8_t> rom, ScreenId screen_id) {
    LoadedTownScene scene{};
    scene.descriptor = load_screen_descriptor(rom, screen_id);
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

    // 7: Primary Screen map layout stream (e.g. 0x212CBA Beach, 0x213850 Town, 0x213FE8 Town North)
    scene.stream_chunks[6] = decompress_rom_stream(rom, scene.descriptor.primary_stream_pointer());
    const auto& map_bytes = scene.stream_chunks[6].decoded_bytes;
    if (map_bytes.size() >= 4U) {
        const std::size_t meta_count = (map_bytes.size() < 10000U) ? 2048U : 4096U;
        if (map_bytes.size() >= 4U + meta_count * 2U) {
            scene.plane_a_metatiles.resize(meta_count);
            for (std::size_t i = 0; i < meta_count; ++i) {
                scene.plane_a_metatiles[i] = read_be16(map_bytes, 4U + i * 2U);
            }
        }
        const std::size_t coll_offset = 4U + meta_count * 2U;
        if (map_bytes.size() >= coll_offset + meta_count) {
            scene.terrain_collision.assign(map_bytes.begin() + static_cast<std::ptrdiff_t>(coll_offset),
                                           map_bytes.begin() + static_cast<std::ptrdiff_t>(coll_offset + meta_count));
        }
        const std::size_t pal_offset = coll_offset + meta_count;
        if (map_bytes.size() >= pal_offset + 64U) {
            for (std::size_t i = 0; i < 32U; ++i) {
                scene.scene_palette_words[i] = read_be16(map_bytes, pal_offset + i * 2U);
            }
        }
    }

    if (screen_id.group == 1U && screen_id.index == 3U) {
        // Slot 4 in Town scene: Town Archway 0x0060 at (160, 120) from 0x046512
        NativeEntity arch_obj{};
        arch_obj.slot_index = 4U;
        arch_obj.type_id = 0x0060U;
        arch_obj.world_x = static_cast<std::int32_t>(read_be16(rom, kTownChestSpawnXAddr));
        arch_obj.world_y = static_cast<std::int32_t>(read_be16(rom, kTownChestSpawnXAddr + 2U));
        arch_obj.x_fp = arch_obj.world_x * 0x10000;
        arch_obj.y_fp = arch_obj.world_y * 0x10000;
        arch_obj.tile_base = 0x8490U;
        arch_obj.half_width = 24;
        arch_obj.half_depth = 12;
        arch_obj.height = 16;
        arch_obj.shadow_idx = 0U;
        arch_obj.flags3a = 0x07U;
        arch_obj.flags37 = 0x64U;
        arch_obj.flags38 = 0x88U;
        arch_obj.flags36 = 0x08U;
        arch_obj.frame_dir = 0U;
        arch_obj.overlay_3e = 0xFFFFU;
        arch_obj.active = true;
        scene.entities.push_back(arch_obj);

        // Slots 5..18: ALL 14 Enemy 0x002C guards from ROM 0x046522..0x04661D
        for (std::size_t idx = 0; idx < kTownEnemySpawnCount; ++idx) {
            const std::size_t rec = kTownEnemySpawnListAddr + idx * kTownEnemyRecordStride;
            const std::uint16_t marker = read_be16(rom, rec);
            if (marker == 0U) {
                break;
            }
            scene.entities.push_back(make_guard_from_rom(rom, rec, static_cast<std::uint16_t>(5U + idx)));
        }
    }

    return scene;
}

LoadedTownScene load_town_scene_from_rom(std::span<const std::uint8_t> rom) {
    return load_world_screen_from_rom(rom, ScreenId{1U, 3U});
}

void upload_town_tiles_to_vdp(const LoadedTownScene& scene, genesis::VdpState& vdp) {
    // 1. Background 4bpp tilesets (chunks 0..3: 4 x 4KB) -> VRAM 0x0000..0x3FFF (0x00D4C8)
    for (std::size_t i = 0; i < 4U; ++i) {
        const std::size_t dst = i * 4096U;
        const auto& bytes = scene.stream_chunks[i].decoded_bytes;
        const std::size_t copy_len = std::min(bytes.size(), vdp.vram.size() - dst);
        if (copy_len > 0U) {
            std::copy_n(bytes.begin(), copy_len, vdp.vram.begin() + static_cast<std::ptrdiff_t>(dst));
        }
    }

    // 2. Entity 0x002C sprite bank (chunk 4: 0x2AF51E) -> VRAM 0x4000 (tile 0x0200)
    {
        constexpr std::size_t kGuardVramAddr = 0x4000U;
        const auto& bytes = scene.stream_chunks[4].decoded_bytes;
        const std::size_t copy_len = std::min(bytes.size(), vdp.vram.size() - kGuardVramAddr);
        if (copy_len > 0U) {
            std::copy_n(bytes.begin(), copy_len, vdp.vram.begin() + static_cast<std::ptrdiff_t>(kGuardVramAddr));
        }
    }

    // 3. Entity 0x0060 Archway sprite bank (chunk 5: 0x163BDA) -> VRAM 0x9200 (tile 0x0490)
    {
        constexpr std::size_t kArchVramAddr = 0x9200U;
        const auto& bytes = scene.stream_chunks[5].decoded_bytes;
        const std::size_t copy_len = std::min(bytes.size(), vdp.vram.size() - kArchVramAddr);
        if (copy_len > 0U) {
            std::copy_n(bytes.begin(), copy_len, vdp.vram.begin() + static_cast<std::ptrdiff_t>(kArchVramAddr));
        }
    }

    // 4. Upload scene palette words ($FF179E -> $FF138C.. / CRAM Pal 2..3)
    for (std::size_t i = 0; i < 32U; ++i) {
        const std::size_t cram_off = (32U + i) * 2U;
        if (cram_off + 1U < vdp.cram.size()) {
            const std::uint16_t w = scene.scene_palette_words[i];
            vdp.cram[cram_off] = static_cast<std::uint8_t>((w >> 8U) & 0xFFU);
            vdp.cram[cram_off + 1U] = static_cast<std::uint8_t>(w & 0xFFU);
        }
    }
}

void populate_plane_a_from_scene(const LoadedTownScene& scene, int cam_x, int cam_y,
                                 int vscroll_a, genesis::VdpState& vdp, TilemapStreamer& streamer) {
    if (scene.plane_a_metatiles.empty()) {
        return;
    }
    const int map_cols = 64;
    const int map_rows = (scene.plane_a_metatiles.size() <= 2048U) ? 32 : 64;
    const int map_col0 = (cam_x / 8) & (map_cols - 1);
    const int map_row0 = (cam_y / 8) & (map_rows - 1);
    const int vram_col0 = map_col0;
    const int vram_row0 = ((vscroll_a / 8) & 31);
    const auto nt_off = static_cast<std::uint16_t>((vram_row0 * 128) | (vram_col0 * 2));
    for (int r = 0; r < 32; ++r) {
        const int mr = (map_row0 + r) & (map_rows - 1);
        const int vr = (vram_row0 + r) & 31;
        for (int c = 0; c < 64; ++c) {
            const int mc = (map_col0 + c) & (map_cols - 1);
            const int vc = (vram_col0 + c) & 63;
            const std::size_t idx = static_cast<std::size_t>(mr) * static_cast<std::size_t>(map_cols) + static_cast<std::size_t>(mc);
            const std::uint16_t word = (idx < scene.plane_a_metatiles.size()) ? scene.plane_a_metatiles[idx] : 0U;
            const std::size_t addr = TilemapStreamer::kPlaneVramBase + (static_cast<std::size_t>(vr) * 64U + static_cast<std::size_t>(vc)) * 2U;
            if (addr + 1U < vdp.vram.size()) {
                vdp.vram[addr] = static_cast<std::uint8_t>((word >> 8U) & 0xFFU);
                vdp.vram[addr + 1U] = static_cast<std::uint8_t>(word & 0xFFU);
            }
        }
    }
    streamer.initialize(cam_x, cam_y, nt_off, scene.plane_a_metatiles);
}

void initialize_native_vdp_from_rom(std::span<const std::uint8_t> rom, const LoadedTownScene& scene,
                                    int cam_x, int cam_y, int vscroll_a,
                                    genesis::VdpState& vdp, TilemapStreamer& streamer) {
    if (!vdp.is_display_enabled()) {
        vdp.registers[0] = 0x04U;
        vdp.registers[1] = 0x74U; // Display enabled, VInt, DMA, V28
        vdp.registers[2] = 0x30U; // Plane A @ 0xC000
        vdp.registers[3] = 0x2CU; // Window @ 0xB000
        vdp.registers[4] = 0x07U; // Plane B @ 0xE000
        vdp.registers[5] = 0x68U; // SAT @ 0xD000
        vdp.registers[7] = 0x00U;
        vdp.registers[11] = 0x00U;
        vdp.registers[12] = 0x81U; // H40 (320px)
        vdp.registers[13] = 0x37U; // HScroll @ 0xDC00
        vdp.registers[15] = 0x02U; // Auto-inc 2
        vdp.registers[16] = 0x01U; // 64x32 planes
    }
    if (rom.size() >= 0x05D0A6U) {
        // Authentic ROM Palettes 0 (0x000474) and 1 (0x05D086)
        std::copy_n(rom.begin() + 0x000474, 32U, vdp.cram.begin());
        std::copy_n(rom.begin() + 0x05D086, 32U, vdp.cram.begin() + 32);
    }
    if (rom.size() >= 0x160000U) {
        // ROM 0x0037BE (0x15517E -> 0xA900 effects/shadows/sparks/ripples),
        // ROM 0x00372A (0x15335A -> 0xB800), ROM 0x00376A (0x15457A -> 0xDB80),
        // ROM 0x00377E (0x1545C8 -> 0xF000 HUD), and initial Ali bank (0x0C83A0 -> 0x9500)
        const auto eff = decompress_rom_stream(rom, 0x15517EU);
        std::copy_n(eff.decoded_bytes.begin(), std::min(eff.decoded_bytes.size(), vdp.vram.size() - 0xA900U), vdp.vram.begin() + 0xA900);
        std::copy_n(rom.begin() + 0x15335A, 0x800U, vdp.vram.begin() + 0xB800);
        const auto db8 = decompress_rom_stream(rom, 0x15457AU);
        std::copy_n(db8.decoded_bytes.begin(), std::min(db8.decoded_bytes.size(), vdp.vram.size() - 0xDB80U), vdp.vram.begin() + 0xDB80);
        const auto hud = decompress_rom_stream(rom, 0x1545C8U);
        std::copy_n(hud.decoded_bytes.begin(), std::min(hud.decoded_bytes.size(), vdp.vram.size() - 0xF000U), vdp.vram.begin() + 0xF000);
        std::copy_n(rom.begin() + 0x0C83A0, 0x880U, vdp.vram.begin() + 0x9500);
    }
    upload_town_tiles_to_vdp(scene, vdp);
    populate_plane_a_from_scene(scene, cam_x, cam_y, vscroll_a, vdp, streamer);
}

} // namespace oasis::game::world