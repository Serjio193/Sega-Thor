#include "game/render/town_combat_sat.hpp"

#include "game/render/player_sprite_composer.hpp"

namespace oasis::game::render {

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

} // namespace oasis::game::render
