#include "game/render/beach_sat_emitter.hpp"

#include "game/render/dytto_sat_renderer.hpp"
#include "game/render/player_sprite_composer.hpp"

namespace oasis::game::render {

void emit_beach_sat(SatPipelineContract& sat,
                    HudOverlay& hud,
                    const camera::CameraController& camera,
                    const player::PlayerState& player,
                    std::uint16_t player_x,
                    std::uint16_t player_y,
                    std::size_t current_pose,
                    int walk_tick,
                    std::span<const NativeEntity> entities,
                    const spirits::DyttoSpiritController* dytto) noexcept {
    sat.reset_frame();

    // 1. Authentic HUD overlay from ROM routine 0x003FA0 (Slots 0..5)
    hud.emit_hud(sat, player.hp, player.max_hp, 100, 100);

    // 2. Beach water reflections (Slots 6..8): ROM routine 0x00A342
    sat.emit_piece(112, 100, 4, 1, 0x079BU, 0, true, false, false);
    sat.emit_piece(144, 100, 4, 1, 0x079FU, 0, true, false, false);
    sat.emit_piece(176, 100, 4, 1, 0x07A3U, 0, true, false, false);

    // 3. Player Ali composite sprite
    const bool facing_left = (player.direction_code == 3);
    const int ax = (current_pose >= 6U)
        ? camera.world_to_screen_x(player_x)
        : camera.world_to_display_x(player_x);
    const int ay = camera.world_to_display_y(player_y);

    if (player.movement_state == 0x30U || player.movement_state == 0x0AU || player.movement_state == 0x46U) {
        PlayerSpriteComposer::emit_attack_pieces(sat, ax, ay, player.direction_code);
    } else {
        const bool alt_step = (walk_tick % 2U) != 0U;
        PlayerSpriteComposer::emit_player_pieces(sat, ax, ay, current_pose, facing_left, alt_step);
    }

    // 4. Beach Chest composite
    for (const auto& entity : entities) {
        if (!entity.active || entity.type_id != 0x0060U) continue;
        const int cx = camera.world_to_display_x(entity.world_x);
        const int cy = camera.world_to_display_y(entity.world_y);
        if (entity.state == 0x6AU) {
            sat.emit_piece(cx, cy, 4, 2, 0x04B0U, 0, false, false, false);
        } else {
            sat.emit_piece(cx - 24, cy - 24, 3, 4, 0x0479U, 0, true, false, false);
            sat.emit_piece(cx - 24, cy - 40, 3, 2, 0x0485U, 0, true, false, false);
            sat.emit_piece(cx - 16, cy - 56, 2, 2, 0x048BU, 0, true, false, false);
            sat.emit_piece(cx - 24, cy - 48, 1, 1, 0x048FU, 0, true, false, false);
            sat.emit_piece(cx + 0, cy - 24, 3, 4, 0x0479U, 0, true, true, false);
            sat.emit_piece(cx + 0, cy - 40, 3, 2, 0x0485U, 0, true, true, false);
            sat.emit_piece(cx + 0, cy - 56, 2, 2, 0x048BU, 0, true, true, false);
            sat.emit_piece(cx + 16, cy - 48, 1, 1, 0x048FU, 0, true, true, false);
        }
    }

    // 5. Pier entities
    for (const auto& entity : entities) {
        if (!entity.active || (entity.type_id != 0x0098U && entity.type_id != 0x0097U)) continue;
        const bool refl_active = (((entity.anim_frame % 4U) == 0U) || ((entity.anim_frame % 4U) == 3U));
        if (refl_active) {
            const int rx = camera.world_to_display_x(222);
            const int ry = camera.world_to_display_y(153);
            if (rx >= -24 && rx <= 320 && ry >= -24 && ry <= 224) {
                sat.emit_piece(rx, ry, 3, 3, 0x0694U, 0, false, false, false);
            }
        }
        const int ex = camera.world_to_display_x(entity.world_x);
        const int ey = camera.world_to_display_y(entity.world_y);
        if (ex >= -24 && ex <= 320 && ey >= -24 && ey <= 224) {
            sat.emit_piece(ex, ey, 3, 3, entity.tile_base, 0, false, false, false);
        }
    }

    // 6. Active Dytto spirit sprite (State 0x1A appear or State 0x04 follow)
    if (dytto != nullptr) {
        emit_dytto_sat(sat, camera, *dytto);
    }

    sat.finalize_frame();
}

} // namespace oasis::game::render