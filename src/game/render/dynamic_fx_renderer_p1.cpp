#include "game/render/dynamic_fx_renderer_p1.hpp"

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

std::optional<CompositorSatEntry> step_aura_fx(
    DynamicFxEntityState& entity,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::uint16_t current_sprites) noexcept {
    if (current_sprites >= DynamicFxP1Config::kMaxSatSprites) {
        return std::nullopt;
    }

    // Step counter at +0x9B wrapping at 80 (0x50)
    entity.aura_counter_9b += 1U;
    if (entity.aura_counter_9b >= 80U) {
        entity.aura_counter_9b = 0U;
    }

    const std::size_t idx = static_cast<std::size_t>((entity.aura_counter_9b >> 2) & 0x0FU);
    const auto& entry = kAuraTable[idx];

    if (entry.tile_or_flag == 255U) {
        return std::nullopt; // Frame skipped
    }

    const std::int16_t screen_x = static_cast<std::int16_t>(
        entity.world_x - cam_x + DynamicFxP1Config::kCoordBias + entry.dx);
    const std::int16_t screen_y = static_cast<std::int16_t>(
        (entity.world_y - cam_y + DynamicFxP1Config::kCoordBias) - entity.elev_z + entry.dy);

    if (is_dynamic_fx_clipped(screen_y, screen_x)) {
        return std::nullopt;
    }

    CompositorSatEntry sat{};
    sat.y = screen_y;
    sat.size = 0x00U; // 1x1
    sat.link = static_cast<std::uint8_t>(current_sprites + 1U);
    sat.tile_attr = 0x8500U | static_cast<std::uint16_t>(entry.tile_or_flag);
    sat.x = screen_x;
    return sat;
}

std::optional<CompositorSatEntry> step_wave_ring_fx(
    DynamicFxEntityState& entity,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::uint16_t current_sprites) noexcept {
    if (current_sprites >= DynamicFxP1Config::kMaxSatSprites) {
        return std::nullopt;
    }

    FxTableEntry entry{};
    if (entity.shadow_preset_3d <= 8U) {
        entity.wave_counter_98 += 1U;
        if (entity.wave_counter_98 >= 8U) {
            entity.wave_counter_98 = 0U;
        }
        entry = kWaveSmallTable[entity.wave_counter_98 % 8U];
    } else {
        entity.wave_counter_98 += 1U;
        if (entity.wave_counter_98 >= 10U) {
            entity.wave_counter_98 = 0U;
        }
        entry = kWaveLargeTable[entity.wave_counter_98 % 10U];
    }

    // Position at ground plane
    const std::int16_t screen_x = static_cast<std::int16_t>(
        entity.world_x - cam_x + DynamicFxP1Config::kCoordBias + entry.dx);
    const std::int16_t screen_y = static_cast<std::int16_t>(
        entity.world_y - cam_y + DynamicFxP1Config::kCoordBias + entity.ground_h + entry.dy);

    if (is_dynamic_fx_clipped(screen_y, screen_x)) {
        return std::nullopt;
    }

    CompositorSatEntry sat{};
    sat.y = screen_y;
    sat.size = 0x05U; // 2x2
    sat.link = static_cast<std::uint8_t>(current_sprites + 1U);
    sat.tile_attr = 0x8500U | static_cast<std::uint16_t>(entry.tile_or_flag);
    sat.x = screen_x;
    return sat;
}

std::optional<CompositorSatEntry> step_ground_ripple_fx(
    DynamicFxEntityState& entity,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::uint16_t current_sprites) noexcept {
    if (current_sprites >= DynamicFxP1Config::kMaxSatSprites) {
        return std::nullopt;
    }

    const std::size_t idx = entity.wave_counter_98 % 8U;
    const auto entry = kRippleTable[idx];

    entity.wave_counter_98 += 1U;
    if (entity.wave_counter_98 >= 8U) {
        entity.wave_counter_98 = 0U;
        entity.fx_flags_39 = static_cast<std::uint8_t>(
            entity.fx_flags_39 & ~DynamicFxP1Config::kGroundRippleBitMask);
    }

    // Position at ground plane
    const std::int16_t screen_x = static_cast<std::int16_t>(
        entity.world_x - cam_x + DynamicFxP1Config::kCoordBias + entry.dx);
    const std::int16_t screen_y = static_cast<std::int16_t>(
        entity.world_y - cam_y + DynamicFxP1Config::kCoordBias + entity.ground_h + entry.dy);

    if (is_dynamic_fx_clipped(screen_y, screen_x)) {
        return std::nullopt;
    }

    CompositorSatEntry sat{};
    sat.y = screen_y;
    sat.size = 0x05U; // 2x2
    sat.link = static_cast<std::uint8_t>(current_sprites + 1U);
    sat.tile_attr = 0x8500U | static_cast<std::uint16_t>(entry.tile_or_flag);
    sat.x = screen_x;
    return sat;
}

DynamicFxP1Result dispatch_dynamic_fx_p1(
    DynamicFxEntityState& entity,
    std::uint8_t global_suppress_flag,
    std::int16_t cam_x,
    std::int16_t cam_y,
    std::uint16_t current_sprites,
    std::uint16_t current_sat_offset) {
    DynamicFxP1Result result{};
    result.final_sprite_count = current_sprites;
    result.final_sat_offset = current_sat_offset;

    // Master should-render check
    if (!should_render_dynamic_fx(global_suppress_flag, entity.fx_flags_39)) {
        result.suppressed = true;
        result.updated_flags_39 = entity.fx_flags_39;
        return result;
    }

    // 1. Bit 0: Aura / Spark Emitter
    if (has_aura_fx(entity.fx_flags_39)) {
        result.emitted_aura = step_aura_fx(entity, cam_x, cam_y, result.final_sprite_count);
        if (result.emitted_aura.has_value()) {
            result.final_sprite_count += 1U;
            result.final_sat_offset = static_cast<std::uint16_t>(result.final_sat_offset + 8U);
        }
    }

    // 2. Bit 3: Wave Ring Emitter
    if (has_wave_ring_fx(entity.fx_flags_39)) {
        result.emitted_wave = step_wave_ring_fx(entity, cam_x, cam_y, result.final_sprite_count);
        if (result.emitted_wave.has_value()) {
            result.final_sprite_count += 1U;
            result.final_sat_offset = static_cast<std::uint16_t>(result.final_sat_offset + 8U);
        }
    }

    // 3. Bit 4: Ground Ripple Emitter
    if (has_ground_ripple_fx(entity.fx_flags_39)) {
        const std::uint8_t initial_flags = entity.fx_flags_39;
        result.emitted_ripple = step_ground_ripple_fx(entity, cam_x, cam_y, result.final_sprite_count);
        if (result.emitted_ripple.has_value()) {
            result.final_sprite_count += 1U;
            result.final_sat_offset = static_cast<std::uint16_t>(result.final_sat_offset + 8U);
        }
        if ((initial_flags & DynamicFxP1Config::kGroundRippleBitMask) &&
            !(entity.fx_flags_39 & DynamicFxP1Config::kGroundRippleBitMask)) {
            result.ripple_completed = true;
        }
    }

    result.updated_flags_39 = entity.fx_flags_39;
    return result;
}

DynamicFxP1Result dispatch_dynamic_fx_p1_raw(
    std::span<std::uint8_t> ram_buffer,
    std::uint32_t entity_addr,
    std::uint32_t ram_base_addr) {
    auto ram_offset = [ram_base_addr, buf_size = ram_buffer.size()](std::uint32_t addr) -> std::size_t {
        if (addr < ram_base_addr || (addr - ram_base_addr) >= buf_size) {
            return buf_size;
        }
        return static_cast<std::size_t>(addr - ram_base_addr);
    };

    DynamicFxP1Result result{};
    const std::size_t ent_pos = ram_offset(entity_addr);
    if (ent_pos + 0xA0 > ram_buffer.size()) {
        result.suppressed = true;
        return result;
    }

    const std::size_t supp_pos = ram_offset(DynamicFxP1Config::kGlobalFxSuppressionRam);
    const std::uint8_t global_supp = (supp_pos < ram_buffer.size()) ? ram_buffer[supp_pos] : 0U;

    DynamicFxEntityState ent{};
    ent.type = read_i16_be(ram_buffer, ent_pos + DynamicFxP1Config::kTypeOffset);
    ent.world_x = read_i16_be(ram_buffer, ent_pos + DynamicFxP1Config::kWorldXOffset);
    ent.world_y = read_i16_be(ram_buffer, ent_pos + DynamicFxP1Config::kWorldYOffset);
    ent.elev_z = read_i16_be(ram_buffer, ent_pos + DynamicFxP1Config::kElevZOffset);
    ent.ground_h = read_i16_be(ram_buffer, ent_pos + DynamicFxP1Config::kGroundHOffset);
    ent.fx_flags_39 = ram_buffer[ent_pos + DynamicFxP1Config::kFxModeFlagsOffset];
    ent.shadow_preset_3d = ram_buffer[ent_pos + DynamicFxP1Config::kShadowPresetOffset];
    ent.wave_counter_98 = ram_buffer[ent_pos + DynamicFxP1Config::kWaveRippleCounterOffset];
    ent.aura_counter_9b = ram_buffer[ent_pos + DynamicFxP1Config::kAuraCounterOffset];

    const std::size_t link_pos = ram_offset(DynamicFxP1Config::kSatLinkCountRam);
    const std::uint16_t cur_sprites = (link_pos < ram_buffer.size()) ?
                                      read_u16_be(ram_buffer, link_pos) : 0U;

    const std::size_t sat_off_pos = ram_offset(DynamicFxP1Config::kSatByteOffsetRam);
    const std::uint16_t cur_sat_off = (sat_off_pos < ram_buffer.size()) ?
                                      read_u16_be(ram_buffer, sat_off_pos) : 0U;

    // Camera Plane selection
    const std::size_t cam_sel_pos = ram_offset(0x00FF16F0U);
    const std::uint8_t cam_sel = (cam_sel_pos < ram_buffer.size()) ? ram_buffer[cam_sel_pos] : 0U;
    const std::uint32_t cam_addr = (cam_sel & 0x80U) ?
                                   DynamicFxP1Config::kCameraPlane2Ram :
                                   DynamicFxP1Config::kCameraPlane1Ram;

    const std::size_t cam_pos = ram_offset(cam_addr);
    const std::int16_t cam_x = (cam_pos < ram_buffer.size()) ? read_i16_be(ram_buffer, cam_pos) : 0;
    const std::int16_t cam_y = (cam_pos + 2 < ram_buffer.size()) ? read_i16_be(ram_buffer, cam_pos + 2) : 0;

    result = dispatch_dynamic_fx_p1(ent, global_supp, cam_x, cam_y, cur_sprites, cur_sat_off);

    // Write back updated counters & flags to entity slot
    ram_buffer[ent_pos + DynamicFxP1Config::kFxModeFlagsOffset] = ent.fx_flags_39;
    ram_buffer[ent_pos + DynamicFxP1Config::kWaveRippleCounterOffset] = ent.wave_counter_98;
    ram_buffer[ent_pos + DynamicFxP1Config::kAuraCounterOffset] = ent.aura_counter_9b;

    // Helper to emit SAT entry into buffer
    auto emit_sat = [&](const CompositorSatEntry& sp, std::uint16_t& off) {
        const std::size_t sat_buf_pos = ram_offset(DynamicFxP1Config::kVdpSatBufferRam);
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
    if (result.emitted_aura.has_value()) {
        emit_sat(*result.emitted_aura, write_offset);
    }
    if (result.emitted_wave.has_value()) {
        emit_sat(*result.emitted_wave, write_offset);
    }
    if (result.emitted_ripple.has_value()) {
        emit_sat(*result.emitted_ripple, write_offset);
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
