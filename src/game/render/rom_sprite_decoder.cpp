#include "game/render/rom_sprite_decoder.hpp"

#include <algorithm>
#include <array>
#include <vector>

namespace oasis::game::render {

namespace {

constexpr std::uint32_t kHudTableAddr = 0x00A438U;
constexpr std::uint32_t kShadowTableAddr = 0x00ACFEU;
constexpr std::uint32_t kTileCountTableAddr = 0x00AD56U;
constexpr std::uint32_t kRippleIdxTableAddr = 0x00B660U;
constexpr std::uint32_t kRipplePtrAddr = 0x03F326U;
constexpr std::uint32_t kEntityClassTableAddr = 0x03F2FAU;

std::uint16_t read_be16(std::span<const std::uint8_t> d, std::size_t off) noexcept {
    if (off + 2U > d.size()) {
        return 0U;
    }
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(d[off]) << 8U) | d[off + 1U]);
}

std::int16_t read_be16s(std::span<const std::uint8_t> d, std::size_t off) noexcept {
    return static_cast<std::int16_t>(read_be16(d, off));
}

std::uint32_t read_be32(std::span<const std::uint8_t> d, std::size_t off) noexcept {
    if (off + 4U > d.size()) {
        return 0U;
    }
    return (static_cast<std::uint32_t>(d[off]) << 24U) |
           (static_cast<std::uint32_t>(d[off + 1U]) << 16U) |
           (static_cast<std::uint32_t>(d[off + 2U]) << 8U) |
           static_cast<std::uint32_t>(d[off + 3U]);
}

int sign_ext8(std::uint8_t v) noexcept {
    return static_cast<int>(static_cast<std::int8_t>(v));
}

void emit_descriptor_parts(SatPipelineContract& sat,
                           std::span<const std::uint8_t> rom,
                           std::uint32_t desc_tbl_addr,
                           std::uint16_t frame_dir,
                           std::uint16_t vram_base,
                           bool is_dynamic_vram,
                           bool hflip,
                           int base_vdp_x,
                           int base_vdp_y) noexcept {
    if (desc_tbl_addr == 0U || frame_dir == 0xFFFFU) {
        return;
    }
    const std::size_t slot_off = desc_tbl_addr + static_cast<std::size_t>(frame_dir) * 2U;
    const std::uint16_t rel = read_be16(rom, slot_off);
    if (rel == 0U) {
        return;
    }
    const std::size_t frame_ptr = slot_off + rel;
    const std::uint16_t count_m1 = read_be16(rom, frame_ptr);
    const std::size_t count = static_cast<std::size_t>(count_m1 & 0x3FU) + 1U;
    const std::uint16_t hflip_mask = hflip ? 0x0800U : 0x0000U;
    std::uint16_t tile_accum = 0U;

    for (std::size_t k = 0; k < count; ++k) {
        const std::size_t rec = frame_ptr + 2U + k * 6U;
        const std::uint16_t dy_sz = read_be16(rom, rec);
        const std::uint16_t pat = read_be16(rom, rec + 2U);
        const std::uint16_t dx2 = read_be16(rom, rec + 4U);

        const std::uint8_t sz_byte = static_cast<std::uint8_t>((dy_sz >> 8U) & 0xFFU);
        const int dy = sign_ext8(static_cast<std::uint8_t>(dy_sz & 0xFFU));
        const int dx = hflip ? sign_ext8(static_cast<std::uint8_t>(dx2 >> 8U))
                             : sign_ext8(static_cast<std::uint8_t>(dx2 & 0xFFU));

        std::uint16_t attr = 0U;
        if (is_dynamic_vram) {
            attr = static_cast<std::uint16_t>((((pat ^ hflip_mask) & 0xF800U) + tile_accum + vram_base) & 0xFFFFU);
        } else {
            attr = static_cast<std::uint16_t>(((pat + vram_base) ^ hflip_mask) & 0xFFFFU);
        }

        sat.emit_rom_piece(base_vdp_x + dx, base_vdp_y + dy, sz_byte, attr);

        if (is_dynamic_vram) {
            const std::uint16_t tcount = read_be16(rom, kTileCountTableAddr + static_cast<std::size_t>(sz_byte & 0x0FU) * 2U);
            tile_accum = static_cast<std::uint16_t>(tile_accum + tcount);
        }
    }
}

void emit_water_ripple_half(SatPipelineContract& sat,
                            std::span<const std::uint8_t> rom,
                            std::uint16_t ripple_frame,
                            std::uint16_t vram_base,
                            bool hflip,
                            int base_vdp_x,
                            int base_vdp_y) noexcept {
    const std::uint32_t ripple_tbl = read_be32(rom, kRipplePtrAddr);
    const std::uint16_t effective_vram = static_cast<std::uint16_t>((vram_base & 0x8000U) | 0x055EU);
    emit_descriptor_parts(sat, rom, ripple_tbl, ripple_frame, effective_vram, false, hflip,
                          base_vdp_x, base_vdp_y);
}

void emit_shadow(SatPipelineContract& sat,
                 std::span<const std::uint8_t> rom,
                 std::uint8_t shadow_idx,
                 int base_vdp_x,
                 int ground_vdp_y) noexcept {
    const std::size_t off = kShadowTableAddr + ((shadow_idx <= 0x18U) ? shadow_idx : 0U);
    const int dy = read_be16s(rom, off);
    const std::uint8_t sz = static_cast<std::uint8_t>(read_be16(rom, off + 2U) >> 8U);
    const std::uint16_t attr = read_be16(rom, off + 4U);
    const int dx = read_be16s(rom, off + 6U);
    sat.emit_rom_piece(base_vdp_x - dx, ground_vdp_y - dy, sz, attr);
}

} // namespace

void emit_rom_town_sat(SatPipelineContract& sat,
                       std::span<const std::uint8_t> rom,
                       const TownSatFrameContext& ctx,
                       std::span<const NativeEntity> entities) noexcept {
    sat.reset_frame();
    if (rom.size() < 0x300000U) {
        return;
    }

    // 1. HUD Slots 0..5 from ROM 0x00A438
    for (std::size_t i = 0; i < 6U; ++i) {
        const std::size_t rec = kHudTableAddr + i * 8U;
        const std::uint16_t y = read_be16(rom, rec);
        const std::uint8_t sz = static_cast<std::uint8_t>(read_be16(rom, rec + 2U) >> 8U);
        const std::uint16_t attr = read_be16(rom, rec + 4U);
        const std::uint16_t x = static_cast<std::uint16_t>(read_be16(rom, rec + 6U) + ctx.hud_x_offset);
        sat.emit_rom_piece(x, y, sz, attr);
    }

    // 2. Blinking status bar strips (ROM 0x00A7E6..0x00A8D8)
    if ((ctx.tick_counter & 0x0080U) == 0U) {
        if ((ctx.tick_counter & 0x0040U) != 0U) {
            sat.emit_rom_piece(0x00F0, 0x00E4, 0x0C, 0x879BU);
            sat.emit_rom_piece(0x0110, 0x00E4, 0x0C, 0x879FU);
            sat.emit_rom_piece(0x0130, 0x00E4, 0x0C, 0x87A3U);
        } else {
            sat.emit_rom_piece(0x010C, 0x00E4, 0x08, 0x879BU);
            sat.emit_rom_piece(0x0124, 0x00E4, 0x04, 0x879EU);
        }
    }

    // 3. Gather renderable slots (Player as slot 0 + active scene entities) and Y-sort descending
    struct RenderItem {
        std::int32_t world_y;
        std::uint16_t slot_index;
        bool is_player;
        const NativeEntity* ent;
    };
    std::vector<RenderItem> items;
    items.reserve(entities.size() + 1U);
    items.push_back({ctx.player_y, 0U, true, nullptr});
    for (const auto& e : entities) {
        if (e.active && e.type_id > 0U && (e.flags36 & 0x02U) == 0U) {
            items.push_back({e.world_y, e.slot_index, false, &e});
        }
    }

    // ROM 0x00A6EC..0x00A718: higher world_y first; tie breaks in favor of lower slot_index
    std::sort(items.begin(), items.end(), [](const RenderItem& a, const RenderItem& b) {
        if (a.world_y != b.world_y) {
            return a.world_y > b.world_y;
        }
        return a.slot_index < b.slot_index;
    });

    // 4. Emit sorted entities
    for (const auto& it : items) {
        if (it.is_player) {
            const int vdp_x = ctx.player_x - ctx.camera_x + 128;
            const int ground_y = ctx.player_y - ctx.camera_y + 128;
            const int vdp_y = ground_y - ctx.player_z;
            const bool hflip = (ctx.player_facing == 3U);

            // Weapon overlay BEFORE Ali body when (overlay_3e & 0x8100) == 0 (ROM 0x00AB6E..0x00B15E)
            if ((ctx.player_overlay_3e & 0x8100U) == 0U) {
                const std::uint32_t w_tbl = read_be32(rom, kEntityClassTableAddr + 0xA0U - 0x14U);
                emit_descriptor_parts(sat, rom, w_tbl, static_cast<std::uint16_t>(ctx.player_overlay_3e & 0x7FU),
                                      ctx.player_weapon_vram, true, hflip, vdp_x, vdp_y);
            }
            const std::uint32_t ali_tbl = read_be32(rom, kEntityClassTableAddr + 12U);
            emit_descriptor_parts(sat, rom, ali_tbl, ctx.player_frame_dir, ctx.player_vram_base,
                                  (ctx.player_flags36 & 0x01U) != 0U, hflip, vdp_x, vdp_y);
            continue;
        }

        const NativeEntity& e = *it.ent;
        if (e.frame_dir == 0xFFFFU) {
            continue;
        }
        int vdp_x = e.world_x - ctx.camera_x + 128;
        const int ground_y = e.world_y - ctx.camera_y + 128;
        int vdp_y = ground_y - e.world_z;
        // ROM 0x00AB1A: bit 6 of flags36 applies +/-2px alternating frame shake (bit 0 of facing selects X vs Y)
        // Uses 60Hz VBLANK frame counter from $FF1971
        if ((e.flags36 & 0x40U) != 0U) {
            const std::uint32_t fc = (ctx.frame_60hz != 0U) ? ctx.frame_60hz : ctx.tick_counter;
            const int jitter = ((fc & 1U) != 0U) ? 2 : 0;
            if ((e.facing & 1U) != 0U) {
                vdp_x += jitter;
            } else {
                vdp_y += jitter;
            }
        }
        const bool hflip = (e.facing == 3U);

        // Water ripple lower half (ROM 0x00B610) before sprite body
        std::uint16_t ripple_idx = 20U;
        if ((e.flags39 & 0x02U) != 0U && e.world_z == 0) {
            const std::uint8_t idx = static_cast<std::uint8_t>(e.water_ripple_tick % 9U);
            ripple_idx = rom[kRippleIdxTableAddr + idx];
            const int r_ay = vdp_y + ((e.shadow_idx >> 2U) - 6);
            emit_water_ripple_half(sat, rom, ripple_idx, e.tile_base, hflip, vdp_x, r_ay);
        }

        const std::size_t cls_off = kEntityClassTableAddr + static_cast<std::size_t>(e.type_id) * 16U - 0x20U;
        const std::uint32_t desc_tbl = read_be32(rom, cls_off + 12U);
        emit_descriptor_parts(sat, rom, desc_tbl, e.frame_dir, e.tile_base,
                              (e.flags36 & 0x01U) != 0U, hflip, vdp_x, vdp_y);

        // Water ripple upper half (ROM 0x00B66A) after sprite body
        if ((e.flags39 & 0x02U) != 0U && e.world_z == 0) {
            const int r_ay = vdp_y - ((e.shadow_idx >> 2U) + 2);
            emit_water_ripple_half(sat, rom, static_cast<std::uint16_t>(ripple_idx + 1U),
                                   e.tile_base, hflip, vdp_x, r_ay);
        }

        // Shadow (ROM 0x00AA7E..0x00ACA2)
        const bool has_shadow = (e.world_z > 0 || (e.flags36 & 0x04U) != 0U) &&
                                (e.flags36 & 0x08U) == 0U && e.world_z >= 0;
        if (has_shadow) {
            emit_shadow(sat, rom, e.shadow_idx, vdp_x, ground_y);
        }
    }

    sat.finalize_frame();
}

} // namespace oasis::game::render