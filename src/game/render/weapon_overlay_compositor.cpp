#include "game/render/weapon_overlay_compositor.hpp"

#include <algorithm>

namespace oasis::game::render {

namespace {

[[nodiscard]] inline std::int16_t read_i16_be(std::span<const std::uint8_t> buf, std::size_t offset) noexcept {
    if (offset + 1 >= buf.size()) {
        return 0;
    }
    return static_cast<std::int16_t>(
        (static_cast<std::uint16_t>(buf[offset]) << 8) |
        static_cast<std::uint16_t>(buf[offset + 1]));
}

[[nodiscard]] inline std::uint16_t read_u16_be(std::span<const std::uint8_t> buf, std::size_t offset) noexcept {
    if (offset + 1 >= buf.size()) {
        return 0U;
    }
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(buf[offset]) << 8) |
        static_cast<std::uint16_t>(buf[offset + 1]));
}

inline void write_u16_be(std::span<std::uint8_t> buf, std::size_t offset, std::uint16_t val) noexcept {
    if (offset + 1 < buf.size()) {
        buf[offset] = static_cast<std::uint8_t>((val >> 8) & 0xFFU);
        buf[offset + 1] = static_cast<std::uint8_t>(val & 0xFFU);
    }
}

} // namespace

WeaponOverlayResult compose_weapon_overlay(
    WeaponOverlayEntityState& entity,
    std::span<const WeaponOverlayPiece> pieces,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::uint16_t current_sprites,
    std::uint16_t current_sat_offset) {
    WeaponOverlayResult result{};
    result.final_sprite_count = current_sprites;
    result.final_sat_offset = current_sat_offset;

    // 1. Mode routing: bit 9 of mode word (0x00B15E)
    if (requires_weapon_dma(entity.mode_word)) {
        result.path = WeaponOverlayPath::DmaStream;
        // DMA path: checks and updates frame cache at +0x5C
        if (entity.weapon_frame != entity.weapon_frame_cache) {
            entity.weapon_frame_cache = entity.weapon_frame;
            result.dma_queued = true;
        }
    } else {
        result.path = WeaponOverlayPath::NonDmaStatic;
        result.dma_queued = false;
    }

    result.updated_frame_cache = entity.weapon_frame_cache;

    // 2. Coordinate transformation with elevation Z (0x00B1DC)
    result.screen_x = static_cast<std::int16_t>(
        entity.world_x - cam_x + WeaponOverlayCompositorConfig::kCoordBias);
    result.screen_y = static_cast<std::int16_t>(
        (entity.world_y - cam_y + WeaponOverlayCompositorConfig::kCoordBias) - entity.elev_z);

    // 3. Emit hardware sprite pieces with viewport clipping
    for (const auto& piece : pieces) {
        if (result.final_sprite_count >= WeaponOverlayCompositorConfig::kMaxSatSprites) {
            break;
        }

        const std::int16_t py = static_cast<std::int16_t>(result.screen_y + piece.y_offset);
        const std::int16_t px = static_cast<std::int16_t>(result.screen_x + piece.x_offset);

        if (is_weapon_overlay_clipped(py, px)) {
            continue;
        }

        CompositorSatEntry entry{};
        entry.y = py;
        entry.size = piece.size;
        entry.link = static_cast<std::uint8_t>(result.final_sprite_count + 1U);
        entry.tile_attr = static_cast<std::uint16_t>(piece.tile_attr + entity.aux_vram_base);
        entry.x = px;

        result.emitted_pieces.push_back(entry);
        result.final_sprite_count += 1U;
        result.final_sat_offset = static_cast<std::uint16_t>(result.final_sat_offset + 8U);
    }

    return result;
}

WeaponOverlayResult compose_weapon_overlay_raw(
    std::span<std::uint8_t> ram_buffer,
    std::uint32_t entity_addr,
    std::int16_t new_weapon_frame,
    std::span<const WeaponOverlayPiece> pieces,
    std::uint32_t ram_base_addr) {
    auto ram_offset = [ram_base_addr, buf_size = ram_buffer.size()](std::uint32_t addr) -> std::size_t {
        if (addr < ram_base_addr || (addr - ram_base_addr) >= buf_size) {
            return buf_size;
        }
        return static_cast<std::size_t>(addr - ram_base_addr);
    };

    WeaponOverlayResult result{};
    const std::size_t ent_pos = ram_offset(entity_addr);
    if (ent_pos + 0x60 > ram_buffer.size()) {
        return result;
    }

    WeaponOverlayEntityState ent{};
    ent.type = read_i16_be(ram_buffer, ent_pos + WeaponOverlayCompositorConfig::kTypeOffset);
    ent.world_x = read_i16_be(ram_buffer, ent_pos + WeaponOverlayCompositorConfig::kWorldXOffset);
    ent.world_y = read_i16_be(ram_buffer, ent_pos + WeaponOverlayCompositorConfig::kWorldYOffset);
    ent.elev_z = read_i16_be(ram_buffer, ent_pos + WeaponOverlayCompositorConfig::kElevZOffset);
    ent.mode_word = read_u16_be(ram_buffer, ent_pos + WeaponOverlayCompositorConfig::kModeWordOffset);
    ent.aux_vram_base = read_u16_be(ram_buffer, ent_pos + WeaponOverlayCompositorConfig::kAuxVramBaseOffset);
    ent.weapon_frame = new_weapon_frame;
    ent.weapon_frame_cache = read_i16_be(ram_buffer, ent_pos + WeaponOverlayCompositorConfig::kWeaponFrameCacheOffset);

    const std::size_t link_pos = ram_offset(WeaponOverlayCompositorConfig::kSatLinkCountRam);
    const std::uint16_t cur_sprites = (link_pos < ram_buffer.size()) ?
                                      read_u16_be(ram_buffer, link_pos) : 0U;

    const std::size_t sat_off_pos = ram_offset(WeaponOverlayCompositorConfig::kSatByteOffsetRam);
    const std::uint16_t cur_sat_off = (sat_off_pos < ram_buffer.size()) ?
                                      read_u16_be(ram_buffer, sat_off_pos) : 0U;

    // Camera Plane selection via bit 7 of $FF16F0
    const std::size_t cam_sel_pos = ram_offset(0x00FF16F0U);
    const std::uint8_t cam_sel = (cam_sel_pos < ram_buffer.size()) ? ram_buffer[cam_sel_pos] : 0U;
    const std::uint32_t cam_addr = (cam_sel & 0x80U) ?
                                   WeaponOverlayCompositorConfig::kCameraPlane2Ram :
                                   WeaponOverlayCompositorConfig::kCameraPlane1Ram;

    const std::size_t cam_pos = ram_offset(cam_addr);
    const std::int16_t cam_x = (cam_pos < ram_buffer.size()) ? read_i16_be(ram_buffer, cam_pos) : 0;
    const std::int16_t cam_y = (cam_pos + 2 < ram_buffer.size()) ? read_i16_be(ram_buffer, cam_pos + 2) : 0;

    result = compose_weapon_overlay(ent, pieces, cam_x, cam_y, cur_sprites, cur_sat_off);

    // If DMA was queued or cache updated, write back to +0x5C
    if (result.dma_queued) {
        write_u16_be(ram_buffer, ent_pos + WeaponOverlayCompositorConfig::kWeaponFrameCacheOffset,
                     static_cast<std::uint16_t>(ent.weapon_frame_cache));
    }

    // Write emitted pieces to SAT buffer
    const std::size_t sat_buf_pos = ram_offset(WeaponOverlayCompositorConfig::kVdpSatBufferRam);
    std::uint16_t write_offset = cur_sat_off;

    for (const auto& sp : result.emitted_pieces) {
        const std::size_t entry_pos = sat_buf_pos + write_offset;
        if (entry_pos + 8 <= ram_buffer.size()) {
            write_u16_be(ram_buffer, entry_pos, static_cast<std::uint16_t>(sp.y));
            ram_buffer[entry_pos + 2] = sp.size;
            ram_buffer[entry_pos + 3] = sp.link;
            write_u16_be(ram_buffer, entry_pos + 4, sp.tile_attr);
            write_u16_be(ram_buffer, entry_pos + 6, static_cast<std::uint16_t>(sp.x));
            write_offset = static_cast<std::uint16_t>(write_offset + 8U);
        }
    }

    // Write back link count and SAT offset
    if (link_pos < ram_buffer.size()) {
        write_u16_be(ram_buffer, link_pos, result.final_sprite_count);
    }
    if (sat_off_pos < ram_buffer.size()) {
        write_u16_be(ram_buffer, sat_off_pos, result.final_sat_offset);
    }

    return result;
}

} // namespace oasis::game::render
