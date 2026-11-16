#pragma once

#include "game/camera/camera_controller.hpp"
#include "game/render/sat_pipeline_contract.hpp"
#include "game/spirits/dytto_controller.hpp"
#include "genesis/vdp_state.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::game::render {

/// Decompresses Dytto's Nemesis-compressed sprite graphics from ROM 0x00141580
/// using routine 0x00003820 (`decompress_graphics`) and uploads the resulting
/// 77 tiles (2464 bytes) into VDP VRAM at byte offset 0x9A00 (VRAM tile 0x04D0),
/// matching ROM 0x014522 -> 0x01454C.
/// Returns the number of bytes written to VRAM, or 0 on invalid/empty ROM.
std::size_t upload_dytto_tiles_to_vram(
    std::span<const std::uint8_t> rom,
    genesis::VdpState& vdp) noexcept;

/// Emits Dytto's hardware SAT sprite pieces into `sat` using explicit camera origin `(cam_x, cam_y)`.
/// Used by both generic/Beach SAT emission and Town-mode (`emit_rom_town_sat`) rendering.
void emit_dytto_sat_at(
    SatPipelineContract& sat,
    int cam_x,
    int cam_y,
    const spirits::DyttoSpiritController& dytto) noexcept;

/// Emits Dytto's hardware SAT sprite pieces into `sat` when `dytto.is_active()`.
/// Must be called before `sat.finalize_frame()`.
/// - State 0x1A (AppearCutscene): emits Dytto at elevated Z (+16 px), with
///   180f appearance burst piece and 240f palette pulse alternation (0x014F60).
/// - State 0x04 (FollowPlayer) & combat states: emits Dytto with directional H-flip derived
///   from `current_angle()` (0x0150A0), active 0x0024 projectiles, and a ground shadow piece.
void emit_dytto_sat(
    SatPipelineContract& sat,
    const camera::CameraController& camera,
    const spirits::DyttoSpiritController& dytto) noexcept;

} // namespace oasis::game::render
