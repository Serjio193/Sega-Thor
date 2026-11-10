#pragma once

#include "game/camera/camera_controller.hpp"
#include "game/entity/native_entity.hpp"
#include "game/player/player.hpp"
#include "game/render/hud_overlay.hpp"
#include "game/render/rom_sprite_decoder.hpp"
#include "game/render/sat_pipeline_contract.hpp"

#include <cstdint>
#include <span>

namespace oasis::game::render {

// Emits authentic Town Screen(1,3) (MAP_ID=0x005A) SAT entries directly from ROM descriptors
// and live world state (Y-sorted painter's order, multi-part sprites, water ripples, shadows).
void emit_town_combat_sat(SatPipelineContract& sat,
                          std::span<const std::uint8_t> rom,
                          const TownSatFrameContext& frame_ctx,
                          std::span<const NativeEntity> entities) noexcept;

// Legacy M16.22 fallback overload preserved for existing unit tests without loaded ROM slice.
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