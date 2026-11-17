#include "game/render/master_sat_compositor.hpp"

#include <algorithm>
#include <numeric>

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

std::optional<CompositorSatEntry> blit_dynamic_shadow(
    std::int16_t world_x,
    std::int16_t world_y,
    std::int16_t ground_h,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::size_t preset_index,
    std::uint16_t current_sprites) noexcept {
    if (current_sprites >= MasterSatCompositorConfig::kMaxSatSprites) {
        return std::nullopt;
    }

    const std::size_t p_idx = std::min(preset_index, kShadowTable.size() - 1U);
    const auto& preset = kShadowTable[p_idx];

    // Shadow ground plane screen position (0x00ACD0)
    const std::int16_t shadow_screen_y = static_cast<std::int16_t>(
        world_y - cam_y + MasterSatCompositorConfig::kCoordBias + ground_h);
    const std::int16_t shadow_screen_x = static_cast<std::int16_t>(
        world_x - cam_x + MasterSatCompositorConfig::kCoordBias);

    const std::int16_t py = static_cast<std::int16_t>(shadow_screen_y - static_cast<std::int16_t>(preset.y_offset));
    const std::int16_t px = static_cast<std::int16_t>(shadow_screen_x - static_cast<std::int16_t>(preset.x_offset));

    if (is_screen_clipped_y(py) || is_screen_clipped_x(px)) {
        return std::nullopt;
    }

    CompositorSatEntry entry{};
    entry.y = py;
    entry.size = static_cast<std::uint8_t>((preset.size_word >> 8) & 0x0FU);
    entry.link = static_cast<std::uint8_t>(current_sprites + 1U);
    entry.tile_attr = 0x8000U | preset.tile_index;
    entry.x = px;
    return entry;
}

MasterCompositorResult compose_master_entity_sat(
    MasterEntityRenderState& entity,
    std::span<const FrameSpritePiece> pieces,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::uint16_t remaining_quota,
    std::uint16_t current_sprites,
    std::uint16_t current_sat_offset) {
    MasterCompositorResult result{};
    result.final_sprite_count = current_sprites;
    result.final_sat_offset = current_sat_offset;

    // 1. Visibility check: bit 1 of flags_36 (0x00A9A0)
    if ((entity.flags_36 & 0x02U) != 0U) {
        result.early_exited = true;
        return result;
    }

    // 2. Accumulate required tiles and evaluate frame cache / quota (0x00A9D6)
    std::uint16_t total_tiles = 0U;
    for (const auto& piece : pieces) {
        total_tiles = static_cast<std::uint16_t>(
            total_tiles + get_sprite_tile_count(piece.size));
    }

    result.cache_eval = evaluate_frame_cache(
        entity.frame_cache, entity.anim_frame, total_tiles, remaining_quota);

    if (!result.cache_eval.deferred) {
        entity.frame_cache = entity.anim_frame;
    }

    // 3. Coordinate transformation with 3D elevation (0x00AB00)
    project_world_to_screen(entity.world_x, entity.world_y, entity.elev_z,
                            cam_x, cam_y, result.screen_x, result.screen_y);

    // 4. Emit hardware sprite pieces with viewport clipping
    for (const auto& piece : pieces) {
        if (result.final_sprite_count >= MasterSatCompositorConfig::kMaxSatSprites) {
            break;
        }

        const std::int16_t py = static_cast<std::int16_t>(result.screen_y + piece.y_offset);
        const std::int16_t px = static_cast<std::int16_t>(result.screen_x + piece.x_offset);

        if (is_screen_clipped_y(py) || is_screen_clipped_x(px)) {
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

    // 5. Dynamic shadow blit if shadow bit is set (bit 2 of flags_3a)
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

MasterCompositorResult compose_master_entity_sat_raw(
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

    MasterCompositorResult result{};
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
    ent.frame_cache = read_i16_be(ram_buffer, ent_pos + MasterSatCompositorConfig::kFrameCacheOffset);
    ent.flags_36 = ram_buffer[ent_pos + MasterSatCompositorConfig::kFlags36Offset];
    ent.flags_3a = ram_buffer[ent_pos + MasterSatCompositorConfig::kFlags3AOffset];

    const std::size_t quota_pos = ram_offset(MasterSatCompositorConfig::kVramQuotaRam);
    const std::uint16_t quota = (quota_pos < ram_buffer.size()) ?
                                read_u16_be(ram_buffer, quota_pos) : 0U;

    const std::size_t link_pos = ram_offset(MasterSatCompositorConfig::kSatLinkCountRam);
    const std::uint16_t cur_sprites = (link_pos < ram_buffer.size()) ?
                                      read_u16_be(ram_buffer, link_pos) : 0U;

    const std::size_t sat_off_pos = ram_offset(MasterSatCompositorConfig::kSatByteOffsetRam);
    const std::uint16_t cur_sat_off = (sat_off_pos < ram_buffer.size()) ?
                                      read_u16_be(ram_buffer, sat_off_pos) : 0U;

    // Camera Plane
    const std::size_t cam_sel_pos = ram_offset(0x00FF16F0U);
    const std::uint8_t cam_sel = (cam_sel_pos < ram_buffer.size()) ? ram_buffer[cam_sel_pos] : 0U;
    const std::uint32_t cam_addr = (cam_sel & 0x80U) ?
                                   MasterSatCompositorConfig::kCameraPlane2Ram :
                                   MasterSatCompositorConfig::kCameraPlane1Ram;

    const std::size_t cam_pos = ram_offset(cam_addr);
    const std::int16_t cam_x = (cam_pos < ram_buffer.size()) ? read_i16_be(ram_buffer, cam_pos) : 0;
    const std::int16_t cam_y = (cam_pos + 2 < ram_buffer.size()) ? read_i16_be(ram_buffer, cam_pos + 2) : 0;

    result = compose_master_entity_sat(ent, pieces, cam_x, cam_y, quota, cur_sprites, cur_sat_off);

    // If deferred, write 0xFF to $FF1651 (0x00A9E2)
    if (result.cache_eval.deferred) {
        const std::size_t def_pos = ram_offset(MasterSatCompositorConfig::kDeferredFlagRam);
        if (def_pos < ram_buffer.size()) {
            ram_buffer[def_pos] = 0xFFU;
        }
    }

    // Write back updated frame cache
    write_u16_be(ram_buffer, ent_pos + MasterSatCompositorConfig::kFrameCacheOffset,
                 static_cast<std::uint16_t>(ent.frame_cache));

    // Emit pieces to SAT buffer
    const std::size_t sat_buf_pos = ram_offset(MasterSatCompositorConfig::kVdpSatBufferRam);
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

} // namespace oasis::game::render
