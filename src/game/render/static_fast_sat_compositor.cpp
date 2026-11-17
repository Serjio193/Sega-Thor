#include "game/render/static_fast_sat_compositor.hpp"

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

StaticCompositorResult compose_static_entity_sat(
    const MasterEntityRenderState& entity,
    std::span<const FrameSpritePiece> pieces,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::uint16_t current_sprites,
    std::uint16_t current_sat_offset) {
    StaticCompositorResult result{};
    result.final_sprite_count = current_sprites;
    result.final_sat_offset = current_sat_offset;

    // 1. Visibility check: bit 1 of flags_36 (0x00AD76)
    if (!is_static_entity_visible(entity.flags_36)) {
        result.early_exited = true;
        return result;
    }

    // 2. Coordinate calculation with elevation Z (0x00AD90)
    result.screen_x = static_cast<std::int16_t>(
        entity.world_x - cam_x + StaticFastSatCompositorConfig::kCoordBias);
    result.screen_y = static_cast<std::int16_t>(
        (entity.world_y - cam_y + StaticFastSatCompositorConfig::kCoordBias) - entity.elev_z);

    // 3. Emit hardware sprite pieces with viewport clipping
    for (const auto& piece : pieces) {
        if (result.final_sprite_count >= StaticFastSatCompositorConfig::kMaxSatSprites) {
            break;
        }

        const std::int16_t py = static_cast<std::int16_t>(result.screen_y + piece.y_offset);
        const std::int16_t px = static_cast<std::int16_t>(result.screen_x + piece.x_offset);

        if (is_static_viewport_clipped(py, px)) {
            continue;
        }

        CompositorSatEntry entry{};
        entry.y = py;
        entry.size = piece.size;
        entry.link = static_cast<std::uint8_t>(result.final_sprite_count + 1U);
        entry.tile_attr = piece.tile_attr;
        entry.x = px;

        result.emitted_pieces.push_back(entry);
        result.final_sprite_count += 1U;
        result.final_sat_offset = static_cast<std::uint16_t>(result.final_sat_offset + 8U);
    }

    // 4. Dynamic shadow generation via table 0x00ACFE if shadow flag bit 2 is set
    if ((entity.flags_3a & 0x04U) != 0U) {
        const std::size_t preset_idx = static_cast<std::size_t>((entity.flags_3a >> 3) & 0x03U);
        result.emitted_shadow = blit_dynamic_shadow(
            entity.world_x, entity.world_y, entity.ground_h,
            cam_x, cam_y, preset_idx, result.final_sprite_count);

        if (result.emitted_shadow.has_value()) {
            result.final_sprite_count += 1U;
            result.final_sat_offset = static_cast<std::uint16_t>(result.final_sat_offset + 8U);
        }
    }

    return result;
}

FastCompositorResult compose_fast_shadowless_sat(
    std::int16_t world_x,
    std::int16_t world_y,
    std::int16_t elev_z,
    std::span<const FrameSpritePiece> pieces,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::uint16_t current_sprites,
    std::uint16_t current_sat_offset) {
    FastCompositorResult result{};
    result.final_sprite_count = current_sprites;
    result.final_sat_offset = current_sat_offset;

    // Screen coordinate projection (0x00B022)
    result.screen_x = static_cast<std::int16_t>(
        world_x - cam_x + StaticFastSatCompositorConfig::kCoordBias);
    result.screen_y = static_cast<std::int16_t>(
        (world_y - cam_y + StaticFastSatCompositorConfig::kCoordBias) - elev_z);

    // Emit pieces with fast viewport clipping (zero shadow logic)
    for (const auto& piece : pieces) {
        if (result.final_sprite_count >= StaticFastSatCompositorConfig::kMaxSatSprites) {
            break;
        }

        const std::int16_t py = static_cast<std::int16_t>(result.screen_y + piece.y_offset);
        const std::int16_t px = static_cast<std::int16_t>(result.screen_x + piece.x_offset);

        if (is_static_viewport_clipped(py, px)) {
            continue;
        }

        CompositorSatEntry entry{};
        entry.y = py;
        entry.size = piece.size;
        entry.link = static_cast<std::uint8_t>(result.final_sprite_count + 1U);
        entry.tile_attr = piece.tile_attr;
        entry.x = px;

        result.emitted_pieces.push_back(entry);
        result.final_sprite_count += 1U;
        result.final_sat_offset = static_cast<std::uint16_t>(result.final_sat_offset + 8U);
    }

    return result;
}

StaticCompositorResult compose_static_entity_sat_raw(
    std::span<std::uint8_t> ram_buffer,
    std::uint32_t entity_addr,
    std::span<const FrameSpritePiece> pieces,
    std::uint32_t ram_base_addr) {
    auto ram_offset = [ram_base_addr, buf_size = ram_buffer.size()](std::uint32_t addr) -> std::size_t {
        if (addr < ram_base_addr || (addr - ram_base_addr) >= buf_size) {
            return buf_size;
        }
        return static_cast<std::size_t>(addr - ram_base_addr);
    };

    StaticCompositorResult result{};
    const std::size_t ent_pos = ram_offset(entity_addr);
    if (ent_pos + 0x40 > ram_buffer.size()) {
        result.early_exited = true;
        return result;
    }

    MasterEntityRenderState ent{};
    ent.type = read_i16_be(ram_buffer, ent_pos + MasterSatCompositorConfig::kTypeOffset);
    ent.world_x = read_i16_be(ram_buffer, ent_pos + MasterSatCompositorConfig::kWorldXOffset);
    ent.world_y = read_i16_be(ram_buffer, ent_pos + MasterSatCompositorConfig::kWorldYOffset);
    ent.elev_z = read_i16_be(ram_buffer, ent_pos + MasterSatCompositorConfig::kElevZOffset);
    ent.ground_h = read_i16_be(ram_buffer, ent_pos + MasterSatCompositorConfig::kGroundHOffset);
    ent.anim_frame = read_i16_be(ram_buffer, ent_pos + MasterSatCompositorConfig::kAnimFrameOffset);
    ent.flags_36 = ram_buffer[ent_pos + MasterSatCompositorConfig::kFlags36Offset];
    ent.flags_3a = ram_buffer[ent_pos + MasterSatCompositorConfig::kFlags3AOffset];

    const std::size_t link_pos = ram_offset(StaticFastSatCompositorConfig::kSatLinkCountRam);
    const std::uint16_t cur_sprites = (link_pos < ram_buffer.size()) ?
                                      read_u16_be(ram_buffer, link_pos) : 0U;

    const std::size_t sat_off_pos = ram_offset(StaticFastSatCompositorConfig::kSatByteOffsetRam);
    const std::uint16_t cur_sat_off = (sat_off_pos < ram_buffer.size()) ?
                                      read_u16_be(ram_buffer, sat_off_pos) : 0U;

    // Camera Plane
    const std::size_t cam_sel_pos = ram_offset(0x00FF16F0U);
    const std::uint8_t cam_sel = (cam_sel_pos < ram_buffer.size()) ? ram_buffer[cam_sel_pos] : 0U;
    const std::uint32_t cam_addr = (cam_sel & 0x80U) ?
                                   StaticFastSatCompositorConfig::kCameraPlane2Ram :
                                   StaticFastSatCompositorConfig::kCameraPlane1Ram;

    const std::size_t cam_pos = ram_offset(cam_addr);
    const std::int16_t cam_x = (cam_pos < ram_buffer.size()) ? read_i16_be(ram_buffer, cam_pos) : 0;
    const std::int16_t cam_y = (cam_pos + 2 < ram_buffer.size()) ? read_i16_be(ram_buffer, cam_pos + 2) : 0;

    result = compose_static_entity_sat(ent, pieces, cam_x, cam_y, cur_sprites, cur_sat_off);

    // Emit pieces to SAT buffer
    const std::size_t sat_buf_pos = ram_offset(StaticFastSatCompositorConfig::kVdpSatBufferRam);
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

    // Emit dynamic shadow to SAT buffer
    if (result.emitted_shadow.has_value()) {
        const auto& sh = *result.emitted_shadow;
        const std::size_t entry_pos = sat_buf_pos + write_offset;
        if (entry_pos + 8 <= ram_buffer.size()) {
            write_u16_be(ram_buffer, entry_pos, static_cast<std::uint16_t>(sh.y));
            ram_buffer[entry_pos + 2] = sh.size;
            ram_buffer[entry_pos + 3] = sh.link;
            write_u16_be(ram_buffer, entry_pos + 4, sh.tile_attr);
            write_u16_be(ram_buffer, entry_pos + 6, static_cast<std::uint16_t>(sh.x));
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

FastCompositorResult compose_fast_shadowless_sat_raw(
    std::span<std::uint8_t> ram_buffer,
    std::int16_t world_x,
    std::int16_t world_y,
    std::int16_t elev_z,
    std::span<const FrameSpritePiece> pieces,
    std::uint32_t ram_base_addr) {
    auto ram_offset = [ram_base_addr, buf_size = ram_buffer.size()](std::uint32_t addr) -> std::size_t {
        if (addr < ram_base_addr || (addr - ram_base_addr) >= buf_size) {
            return buf_size;
        }
        return static_cast<std::size_t>(addr - ram_base_addr);
    };

    const std::size_t link_pos = ram_offset(StaticFastSatCompositorConfig::kSatLinkCountRam);
    const std::uint16_t cur_sprites = (link_pos < ram_buffer.size()) ?
                                      read_u16_be(ram_buffer, link_pos) : 0U;

    const std::size_t sat_off_pos = ram_offset(StaticFastSatCompositorConfig::kSatByteOffsetRam);
    const std::uint16_t cur_sat_off = (sat_off_pos < ram_buffer.size()) ?
                                      read_u16_be(ram_buffer, sat_off_pos) : 0U;

    // Camera Plane
    const std::size_t cam_sel_pos = ram_offset(0x00FF16F0U);
    const std::uint8_t cam_sel = (cam_sel_pos < ram_buffer.size()) ? ram_buffer[cam_sel_pos] : 0U;
    const std::uint32_t cam_addr = (cam_sel & 0x80U) ?
                                   StaticFastSatCompositorConfig::kCameraPlane2Ram :
                                   StaticFastSatCompositorConfig::kCameraPlane1Ram;

    const std::size_t cam_pos = ram_offset(cam_addr);
    const std::int16_t cam_x = (cam_pos < ram_buffer.size()) ? read_i16_be(ram_buffer, cam_pos) : 0;
    const std::int16_t cam_y = (cam_pos + 2 < ram_buffer.size()) ? read_i16_be(ram_buffer, cam_pos + 2) : 0;

    FastCompositorResult result = compose_fast_shadowless_sat(
        world_x, world_y, elev_z, pieces, cam_x, cam_y, cur_sprites, cur_sat_off);

    // Emit pieces to SAT buffer
    const std::size_t sat_buf_pos = ram_offset(StaticFastSatCompositorConfig::kVdpSatBufferRam);
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
