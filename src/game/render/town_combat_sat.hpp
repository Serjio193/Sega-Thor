#pragma once

#include "game/camera/camera_controller.hpp"
#include "game/entity/native_entity.hpp"
#include "game/player/player.hpp"
#include "game/render/hud_overlay.hpp"
#include "game/render/sat_pipeline_contract.hpp"

#include <cstdint>
#include <span>

namespace oasis::game::render {

// Emits authentic Town Screen(1,3) (MAP_ID=0x005A) SAT entries from live camera,
// HUD, Player Ali, Arch 0x0060, and active Enemy 0x002C world coordinates.
// Zero prerecorded SAT bytes are used.
void emit_town_combat_sat(SatPipelineContract& sat,
                          HudOverlay& hud,
                          const camera::CameraController& camera,
                          const player::PlayerState& player,
                          std::int32_t player_x,
                          std::int32_t player_y,
                          int walk_tick,
                          std::span<const NativeEntity> entities,
                          bool include_status_bars = true) noexcept;

} // namespace oasis::game::render
