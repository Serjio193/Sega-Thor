#include "game/render/town_combat_sat.hpp"

#include "game/render/player_sprite_composer.hpp"

#include <cstring>

namespace oasis::game::render {

void emit_town_combat_sat(SatPipelineContract& sat,
                          std::span<const std::uint8_t> rom,
                          const TownSatFrameContext& frame_ctx,
                          std::span<const NativeEntity> entities,
                          VdpDmaQueue* dma_queue) noexcept {
    emit_rom_town_sat(sat, rom, frame_ctx, entities, dma_queue);
}

void emit_town_combat_sat(SatPipelineContract& sat,
                          HudOverlay& hud,
                          const camera::CameraController& camera,
                          const player::PlayerState& player,
                          std::int32_t player_x,
                          std::int32_t player_y,
                          int walk_tick,
                          std::span<const NativeEntity> entities,
                          bool include_status_bars) noexcept {
    sat.reset_frame();

    // 1. HUD Overlay (Slots 0..5)
    hud.emit_hud(sat, player.hp, player.max_hp, 100, 100);

    // 2. Town Screen overlay status strips (Slots 6..8) when active
    if (include_status_bars) {
        sat.emit_piece(112, 100, 4, 1, 0x079BU, 0, true, false, false);
        sat.emit_piece(144, 100, 4, 1, 0x079FU, 0, true, false, false);
        sat.emit_piece(176, 100, 4, 1, 0x07A3U, 0, true, false, false);
    }

    // 3. Player Ali composite sprite
    const int px = camera.world_to_screen_x(player_x);
    const int py = camera.world_to_screen_y(player_y);
    const bool facing_left = (player.direction_code == 3);
    if (player.movement_state == 0x30U || player.movement_state == 0x0AU ||
        player.movement_state == 0x36U || player.movement_state == 0x46U) {
        PlayerSpriteComposer::emit_attack_pieces(sat, px, py, player.direction_code);
    } else {
        const std::size_t pose = static_cast<std::size_t>(player.direction_code & 3U);
        const bool alt = (walk_tick % 2) != 0;
        PlayerSpriteComposer::emit_player_pieces(sat, px, py, pose, facing_left, alt);
    }

    // 4. Town Entities (Arch 0x0060 + 0x002C Guards with airborne/corpse lifecycle)
    for (const auto& ent : entities) {
        if (!ent.active || ent.type_id == 0U) {
            continue;
        }
        const int ex = camera.world_to_screen_x(ent.world_x);
        const int ey = camera.world_to_screen_y(ent.world_y, ent.world_z);
        const int ground_y = camera.world_to_screen_y(ent.world_y, 0);

        if (ent.type_id == 0x0060U) {
            sat.emit_piece(ex - 24, ground_y - 27, 3, 4, 0x0490U, 0, true, false, false);
            sat.emit_piece(ex, ground_y - 27, 3, 4, 0x0490U, 0, true, true, false);
            continue;
        }

        if (ent.type_id != 0x002CU) {
            continue;
        }
        if ((ent.flags36 & 0x40U) != 0U) {
            continue; // Hurt flash blink frame
        }

        if (ent.state == 0x06U) {
            sat.emit_piece(ex - 12, ey - 26, 3, 4, 0x0295U, 1, false, false, false);
            sat.emit_piece(ex - 11, ground_y - 3, 3, 1, 0x054BU, 0, false, false, false);
        } else if (ent.state == 0x08U) {
            sat.emit_piece(ex, ground_y - 3, 1, 1, 0x05AAU, 0, false, false, false);
            sat.emit_piece(ex - 16, ey - 14, 4, 3, 0x02C1U, 1, false, false, false);
        } else {
            const std::uint16_t pat = (ent.state == 0x04U) ? 0x02A9U : 0x0200U;
            sat.emit_piece(ex - 16, ey - 25, 4, 3, pat, 1, false, ent.facing == 3U, false);
        }
    }

    sat.finalize_frame();
}

void apply_player_patterns_and_water_cycle(std::span<const std::uint8_t> walk_patterns,
                                           std::size_t pose_idx,
                                           std::uint32_t frame,
                                           genesis::VdpState& vdp) noexcept {
    if (!walk_patterns.empty() && walk_patterns.size() >= (pose_idx + 1U) * 1152U) {
        const auto* pose_ptr = walk_patterns.data() + pose_idx * 1152U;
        std::memcpy(&vdp.vram[0x9500], pose_ptr, 1024);
        std::memcpy(&vdp.vram[0x9D00], pose_ptr + 1024, 128);
        if (pose_idx == 3U || pose_idx == 5U) {
            static constexpr std::uint8_t kPose3UpperExt[64] = {
                0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0xB0U, 0x00U, 0x00U,
                0x0BU, 0x3BU, 0x00U, 0x00U, 0x0BU, 0x44U, 0x00U, 0x00U, 0xB1U, 0x2FU, 0x00U, 0x00U, 0x12U, 0xF2U, 0xF0U, 0x00U,
                0xD1U, 0x22U, 0xF4U, 0xB0U, 0xFBU, 0xBBU, 0x34U, 0x4BU, 0xB0U, 0x0BU, 0xBBU, 0x3BU, 0x00U, 0x00U, 0x00U, 0xBBU,
                0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U
            };
            std::memcpy(&vdp.vram[0x9D80], kPose3UpperExt, 64);
            std::memset(&vdp.vram[0x9DC0], 0, 64);
        } else {
            std::memset(&vdp.vram[0x9D80], 0, 128);
        }
    }

    static constexpr std::uint16_t kWaterCycle[4][4] = {
        {0x0820, 0x0EC0, 0x0E40, 0x0820}, {0x0EC0, 0x0E40, 0x0820, 0x0820},
        {0x0E40, 0x0820, 0x0820, 0x0EC0}, {0x0820, 0x0820, 0x0EC0, 0x0E40}
    };
    const std::size_t pal_phase = (frame / 6U) % 4U;
    for (std::size_t i = 0; i < 4U; ++i) {
        const std::size_t cram_off = (33U + i) * 2U;
        const std::uint16_t w = kWaterCycle[pal_phase][i];
        vdp.cram[cram_off] = static_cast<std::uint8_t>((w >> 8U) & 0xFFU);
        vdp.cram[cram_off + 1U] = static_cast<std::uint8_t>(w & 0xFFU);
    }
}

void step_town_water_cycle(std::uint8_t& timer,
                           std::uint8_t& phase,
                           genesis::VdpState& vdp) noexcept {
    static constexpr std::uint16_t kTownWaterCycle[4][4] = {
        {0x0108U, 0x01F0U, 0x01D0U, 0x0108U},
        {0x01F0U, 0x01D0U, 0x0108U, 0x0108U},
        {0x01D0U, 0x0108U, 0x0108U, 0x01F0U},
        {0x0108U, 0x0108U, 0x01F0U, 0x01D0U}
    };
    ++timer;
    if (timer >= 10U) {
        timer = 0U;
        phase = static_cast<std::uint8_t>((phase + 1U) & 0x03U);
    }
    const auto& row = kTownWaterCycle[phase & 0x03U];
    for (std::size_t i = 0; i < 4U; ++i) {
        const std::size_t cram_off = (33U + i) * 2U;
        vdp.cram[cram_off] = static_cast<std::uint8_t>((row[i] >> 8U) & 0xFFU);
        vdp.cram[cram_off + 1U] = static_cast<std::uint8_t>(row[i] & 0xFFU);
    }
}

void build_town_sat_and_dma(SatPipelineContract& sat,
                            std::span<const std::uint8_t> rom,
                            const camera::CameraController& camera,
                            const player::PlayerState& player,
                            std::uint16_t tick_counter,
                            std::uint32_t frame_60hz,
                            std::uint16_t player_x,
                            std::uint16_t player_y,
                            int walk_tick,
                            std::size_t pose_idx,
                            bool attract_banner_enabled,
                            bool advance_ripples,
                            std::span<NativeEntity> entities,
                            VdpDmaQueue& dma_queue) noexcept {
    TownSatFrameContext sat_ctx{};
    sat_ctx.camera_x = camera.camera_x();
    sat_ctx.camera_y = camera.camera_y();
    sat_ctx.tick_counter = tick_counter;
    sat_ctx.frame_60hz = frame_60hz;
    sat_ctx.hud_x_offset = (player_x < 80U) ? 0x138 : 0x20;
    sat_ctx.attract_banner_enabled = attract_banner_enabled;
    sat_ctx.player_x = player_x;
    sat_ctx.player_y = player_y;
    sat_ctx.player_facing = player.direction_code;
    if (player.movement_state == 4U) {
        sat_ctx.player_frame_dir = static_cast<std::uint16_t>(24U + (walk_tick / 2U) * 3U);
        sat_ctx.player_overlay_3e = static_cast<std::uint16_t>(48U + (walk_tick / 2U) * 2U);
    } else if (player.movement_state == 0x36U) {
        sat_ctx.player_frame_dir = 186U;
        sat_ctx.player_overlay_3e = 2328U;
    } else {
        sat_ctx.player_frame_dir = static_cast<std::uint16_t>(pose_idx * 3U);
    }
    if (advance_ripples) {
        for (auto& ent : entities) {
            if (ent.active && (ent.flags39 & 0x02U) != 0U && ent.world_z == 0) {
                ent.water_ripple_tick = static_cast<std::uint8_t>((ent.water_ripple_tick + 1U) % 9U);
            }
        }
    }
    dma_queue.clear();
    emit_town_combat_sat(sat, rom, sat_ctx, entities, &dma_queue);
}

} // namespace oasis::game::render