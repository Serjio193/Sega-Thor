#include "game/render/dynamic_fx_renderer_p2.hpp"

#include <algorithm>

namespace oasis::game::render {

namespace {

// Frame piece storage for frames 4..6 (Halo) and 20..25 (Swirl)
constexpr std::array<FxSpritePiece, 1> kFrame4Pieces = {{{0x00U, -4, 76U, -5, -3}}};
constexpr std::array<FxSpritePiece, 1> kFrame5Pieces = {{{0x00U, -4, 77U, -4, -4}}};
constexpr std::array<FxSpritePiece, 1> kFrame6Pieces = {{{0x00U, -4, 78U, -4, -4}}};

constexpr std::array<FxSpritePiece, 2> kFrame20Pieces = {{
    {0x05U, 0, 2134U, -16, 0},
    {0x05U, 0, 86U, 0, -16}
}};
constexpr std::array<FxSpritePiece, 2> kFrame21Pieces = {{
    {0x04U, -8, 2132U, -14, -2},
    {0x04U, -8, 84U, -1, -15}
}};
constexpr std::array<FxSpritePiece, 2> kFrame22Pieces = {{
    {0x05U, 0, 90U, 0, -16},
    {0x05U, 0, 2138U, -16, 0}
}};
constexpr std::array<FxSpritePiece, 2> kFrame23Pieces = {{
    {0x04U, -9, 2132U, -16, 0},
    {0x04U, -9, 84U, 0, -16}
}};
constexpr std::array<FxSpritePiece, 2> kFrame24Pieces = {{
    {0x05U, -2, 2138U, -14, -2},
    {0x05U, -2, 90U, -1, -15}
}};
constexpr std::array<FxSpritePiece, 2> kFrame25Pieces = {{
    {0x04U, -8, 2132U, -13, -3},
    {0x04U, -8, 84U, -2, -14}
}};

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

std::span<const FxSpritePiece> get_fx_frame_pieces(std::uint8_t frame_index) noexcept {
    switch (frame_index) {
        case 4U:  return kFrame4Pieces;
        case 5U:  return kFrame5Pieces;
        case 6U:  return kFrame6Pieces;
        case 20U: return kFrame20Pieces;
        case 21U: return kFrame21Pieces;
        case 22U: return kFrame22Pieces;
        case 23U: return kFrame23Pieces;
        case 24U: return kFrame24Pieces;
        case 25U: return kFrame25Pieces;
        default:  return {};
    }
}

std::vector<CompositorSatEntry> blit_fx_frame(
    std::uint8_t frame_index,
    std::int16_t screen_x,
    std::int16_t screen_y,
    std::uint16_t base_tile_attr,
    std::uint16_t current_sprites,
    bool flip_x) noexcept {
    const auto pieces = get_fx_frame_pieces(frame_index);
    if (pieces.empty()) {
        return {};
    }

    std::vector<CompositorSatEntry> emitted;
    std::uint16_t sprite_idx = current_sprites;

    for (const auto& piece : pieces) {
        if (sprite_idx >= DynamicFxP2Config::kMaxSatSprites) {
            break;
        }

        const std::int16_t sy = static_cast<std::int16_t>(screen_y + piece.y_offset);
        const std::int16_t sx = static_cast<std::int16_t>(
            screen_x + (flip_x ? piece.x_flip : piece.x_unflip));

        if (is_dynamic_fx_p2_clipped(sy, sx)) {
            continue;
        }

        CompositorSatEntry sat{};
        sat.y = sy;
        sat.size = piece.size;
        sat.link = static_cast<std::uint8_t>(sprite_idx + 1U);
        sat.tile_attr = static_cast<std::uint16_t>(base_tile_attr + piece.tile_attr);
        sat.x = sx;

        emitted.push_back(sat);
        sprite_idx += 1U;
    }

    return emitted;
}

std::vector<CompositorSatEntry> blit_raw_pieces(
    std::span<const FxSpritePiece> pieces,
    std::int16_t screen_x,
    std::int16_t screen_y,
    std::uint16_t base_tile_attr,
    std::uint16_t current_sprites) noexcept {
    std::vector<CompositorSatEntry> emitted;
    std::uint16_t sprite_idx = current_sprites;

    const std::int16_t biased_x = static_cast<std::int16_t>(screen_x + DynamicFxP2Config::kScreenBias);
    const std::int16_t biased_y = static_cast<std::int16_t>(screen_y + DynamicFxP2Config::kScreenBias);

    for (const auto& piece : pieces) {
        if (sprite_idx >= DynamicFxP2Config::kMaxSatSprites) {
            break;
        }

        const std::int16_t sy = static_cast<std::int16_t>(biased_y + piece.y_offset);
        const std::int16_t sx = static_cast<std::int16_t>(biased_x + piece.x_unflip);

        if (is_dynamic_fx_p2_clipped(sy, sx)) {
            continue;
        }

        CompositorSatEntry sat{};
        sat.y = sy;
        sat.size = piece.size;
        sat.link = static_cast<std::uint8_t>(sprite_idx + 1U);
        sat.tile_attr = static_cast<std::uint16_t>(base_tile_attr + piece.tile_attr);
        sat.x = sx;

        emitted.push_back(sat);
        sprite_idx += 1U;
    }

    return emitted;
}

std::vector<CompositorSatEntry> step_halo_fx(
    DynamicFxEntityStateP2& entity,
    bool is_paused,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::uint16_t current_sprites,
    bool& out_completed) noexcept {
    out_completed = false;

    if (!is_paused) {
        if (entity.halo_counter > 0U) {
            entity.halo_counter -= 1U;
        }
    }

    if (entity.halo_counter == 0U) {
        entity.fx_flags_39 = static_cast<std::uint8_t>(
            entity.fx_flags_39 & ~DynamicFxP2Config::kHaloBitMask);
        out_completed = true;
        return {};
    }

    if (entity.halo_counter >= 21U) {
        entity.halo_counter = 0U;
    }

    const auto& entry = kHaloTable[entity.halo_counter % 21U];
    if (entry.frame_flag == 255U) {
        return {}; // Frame skipped
    }

    const std::int16_t sx = static_cast<std::int16_t>(
        entity.world_x - cam_x + DynamicFxP2Config::kScreenBias + entry.dx);
    const std::int16_t sy = static_cast<std::int16_t>(
        entity.world_y - cam_y + DynamicFxP2Config::kScreenBias + entity.elev_z + entry.dy);

    const std::uint16_t base_attr = compute_fx_tile_attr(entity.base_tile_18);
    return blit_fx_frame(entry.frame_flag, sx, sy, base_attr, current_sprites);
}

std::vector<CompositorSatEntry> step_swirl_fx(
    DynamicFxEntityStateP2& entity,
    bool is_paused,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::uint16_t current_sprites,
    bool is_secondary) noexcept {
    // Swirl is only active when grounded (elevation == 0)
    if (entity.elev_z != 0) {
        return {};
    }

    if (!is_paused) {
        entity.swirl_counter += 1U;
        if (entity.swirl_counter >= 9U) {
            entity.swirl_counter = 0U;
        }
    }

    const std::uint8_t base_frame = kSwirlTable[entity.swirl_counter % 9U];
    const std::uint8_t frame_id = is_secondary ? static_cast<std::uint8_t>(base_frame + 1U) : base_frame;
    const std::int16_t dy = is_secondary ?
        compute_secondary_swirl_y_offset(entity.shadow_preset_3d) :
        compute_swirl_y_offset(entity.shadow_preset_3d);

    const std::int16_t sx = static_cast<std::int16_t>(
        entity.world_x - cam_x + DynamicFxP2Config::kScreenBias);
    const std::int16_t sy = static_cast<std::int16_t>(
        entity.world_y - cam_y + DynamicFxP2Config::kScreenBias + dy);

    const std::uint16_t base_attr = compute_fx_tile_attr(entity.base_tile_18);
    return blit_fx_frame(frame_id, sx, sy, base_attr, current_sprites);
}

DynamicFxP2Result dispatch_dynamic_fx_p2(
    DynamicFxEntityStateP2& entity,
    bool is_paused,
    std::uint8_t global_suppress_flag,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::uint16_t current_sprites,
    std::uint16_t current_sat_offset) {
    DynamicFxP2Result result{};
    result.final_sprite_count = current_sprites;
    result.final_sat_offset = current_sat_offset;

    if (global_suppress_flag != 0U || entity.fx_flags_39 == 0U) {
        result.suppressed = true;
        result.updated_flags_39 = entity.fx_flags_39;
        return result;
    }

    // 1. Bit 1: Swirl Emitter (0x00B610)
    if (entity.fx_flags_39 & DynamicFxP2Config::kSwirlBitMask) {
        result.emitted_swirl = step_swirl_fx(
            entity, is_paused, cam_x, cam_y, result.final_sprite_count);
        result.final_sprite_count = static_cast<std::uint16_t>(
            result.final_sprite_count + result.emitted_swirl.size());
        result.final_sat_offset = static_cast<std::uint16_t>(
            result.final_sat_offset + result.emitted_swirl.size() * 8U);
    }

    // 2. Bit 2: Halo Emitter (0x00B56E)
    if (entity.fx_flags_39 & DynamicFxP2Config::kHaloBitMask) {
        bool halo_done = false;
        result.emitted_halo = step_halo_fx(
            entity, is_paused, cam_x, cam_y, result.final_sprite_count, halo_done);
        result.halo_completed = halo_done;
        result.final_sprite_count = static_cast<std::uint16_t>(
            result.final_sprite_count + result.emitted_halo.size());
        result.final_sat_offset = static_cast<std::uint16_t>(
            result.final_sat_offset + result.emitted_halo.size() * 8U);
    }

    result.updated_flags_39 = entity.fx_flags_39;
    return result;
}

DynamicFxP2Result dispatch_dynamic_fx_p2_raw(
    std::span<std::uint8_t> ram_buffer,
    std::uint32_t entity_addr,
    std::uint32_t ram_base_addr) {
    auto ram_offset = [ram_base_addr, buf_size = ram_buffer.size()](std::uint32_t addr) -> std::size_t {
        if (addr < ram_base_addr || (addr - ram_base_addr) >= buf_size) {
            return buf_size;
        }
        return static_cast<std::size_t>(addr - ram_base_addr);
    };

    DynamicFxP2Result result{};
    const std::size_t ent_pos = ram_offset(entity_addr);
    if (ent_pos + 0xA0 > ram_buffer.size()) {
        result.suppressed = true;
        return result;
    }

    const std::size_t supp_pos = ram_offset(DynamicFxP2Config::kGlobalFxSuppressionRam);
    const std::uint8_t global_supp = (supp_pos < ram_buffer.size()) ? ram_buffer[supp_pos] : 0U;

    const std::size_t pause_pos = ram_offset(DynamicFxP2Config::kGlobalPauseFlagRam);
    const bool is_paused = (pause_pos < ram_buffer.size()) && (ram_buffer[pause_pos] != 0U);

    const bool is_secondary = (entity_addr >= DynamicFxP2Config::kSecondaryPoolThreshold);

    DynamicFxEntityStateP2 ent{};
    ent.world_x = read_i16_be(ram_buffer, ent_pos + DynamicFxP2Config::kWorldXOffset);
    ent.world_y = read_i16_be(ram_buffer, ent_pos + DynamicFxP2Config::kWorldYOffset);
    ent.elev_z = read_i16_be(ram_buffer, ent_pos + DynamicFxP2Config::kElevZOffset);
    ent.ground_h = read_i16_be(ram_buffer, ent_pos + DynamicFxP2Config::kGroundHOffset);
    ent.base_tile_18 = read_u16_be(ram_buffer, ent_pos + DynamicFxP2Config::kBaseTileOffset);
    ent.fx_flags_39 = ram_buffer[ent_pos + DynamicFxP2Config::kFxModeFlagsOffset];
    ent.shadow_preset_3d = ram_buffer[ent_pos + DynamicFxP2Config::kShadowPresetOffset];

    const std::size_t halo_cnt_off = is_secondary ?
        DynamicFxP2Config::kSecondaryCounterOffset : DynamicFxP2Config::kHaloCounterOffset;
    ent.halo_counter = ram_buffer[ent_pos + halo_cnt_off];

    const std::size_t swirl_cnt_off = is_secondary ?
        DynamicFxP2Config::kSecondaryCounterOffset : DynamicFxP2Config::kSwirlCounterOffset;
    ent.swirl_counter = ram_buffer[ent_pos + swirl_cnt_off];

    const std::size_t link_pos = ram_offset(DynamicFxP2Config::kSatLinkCountRam);
    const std::uint16_t cur_sprites = (link_pos < ram_buffer.size()) ?
                                      read_u16_be(ram_buffer, link_pos) : 0U;

    const std::size_t sat_off_pos = ram_offset(DynamicFxP2Config::kSatByteOffsetRam);
    const std::uint16_t cur_sat_off = (sat_off_pos < ram_buffer.size()) ?
                                      read_u16_be(ram_buffer, sat_off_pos) : 0U;

    // Camera Plane
    const std::size_t cam_sel_pos = ram_offset(0x00FF16F0U);
    const std::uint8_t cam_sel = (cam_sel_pos < ram_buffer.size()) ? ram_buffer[cam_sel_pos] : 0U;
    const std::uint32_t cam_addr = (cam_sel & 0x80U) ?
                                   DynamicFxP2Config::kCameraPlane2Ram :
                                   DynamicFxP2Config::kCameraPlane1Ram;

    const std::size_t cam_pos = ram_offset(cam_addr);
    const std::int16_t cam_x = (cam_pos < ram_buffer.size()) ? read_i16_be(ram_buffer, cam_pos) : 0;
    const std::int16_t cam_y = (cam_pos + 2 < ram_buffer.size()) ? read_i16_be(ram_buffer, cam_pos + 2) : 0;

    result = dispatch_dynamic_fx_p2(ent, is_paused, global_supp, cam_x, cam_y, cur_sprites, cur_sat_off);

    // Write back counters & flags
    ram_buffer[ent_pos + DynamicFxP2Config::kFxModeFlagsOffset] = ent.fx_flags_39;
    ram_buffer[ent_pos + halo_cnt_off] = ent.halo_counter;
    ram_buffer[ent_pos + swirl_cnt_off] = ent.swirl_counter;

    // Emit SAT entries
    auto emit_sat = [&](const CompositorSatEntry& sp, std::uint16_t& off) {
        const std::size_t sat_buf_pos = ram_offset(DynamicFxP2Config::kVdpSatBufferRam);
        const std::size_t entry_pos = sat_buf_pos + off;
        if (entry_pos + 8 <= ram_buffer.size()) {
            write_u16_be(ram_buffer, entry_pos, static_cast<std::uint16_t>(sp.y));
            ram_buffer[entry_pos + 2] = sp.size;
            ram_buffer[entry_pos + 3] = sp.link;
            write_u16_be(ram_buffer, entry_pos + 4, sp.tile_attr);
            write_u16_be(ram_buffer, entry_pos + 6, static_cast<std::uint16_t>(sp.x));
            off = static_cast<std::uint16_t>(off + 8U);
        }
    };

    std::uint16_t write_offset = cur_sat_off;
    for (const auto& sp : result.emitted_swirl) {
        emit_sat(sp, write_offset);
    }
    for (const auto& sp : result.emitted_halo) {
        emit_sat(sp, write_offset);
    }

    if (link_pos < ram_buffer.size()) {
        write_u16_be(ram_buffer, link_pos, result.final_sprite_count);
    }
    if (sat_off_pos < ram_buffer.size()) {
        write_u16_be(ram_buffer, sat_off_pos, result.final_sat_offset);
    }

    return result;
}

} // namespace oasis::game::render
