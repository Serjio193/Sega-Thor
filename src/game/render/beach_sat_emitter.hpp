#pragma once

#include "game/camera/camera_controller.hpp"
#include "game/entity/native_entity.hpp"
#include "game/player/player.hpp"
#include "game/render/hud_overlay.hpp"
#include "game/render/sat_pipeline_contract.hpp"
#include "game/spirits/dytto_controller.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::game::render {

// Emits authentic Beach Screen(1,2) and generic world SAT entries (HUD, water status bars, Player Ali, Chest, Pier, optional Dytto).
void emit_beach_sat(SatPipelineContract& sat,
                    HudOverlay& hud,
                    const camera::CameraController& camera,
                    const player::PlayerState& player,
                    std::uint16_t player_x,
                    std::uint16_t player_y,
                    std::size_t current_pose,
                    int walk_tick,
                    std::span<const NativeEntity> entities,
                    const spirits::DyttoSpiritController* dytto = nullptr) noexcept;

} // namespace oasis::game::render