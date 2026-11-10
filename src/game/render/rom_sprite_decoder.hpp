#pragma once

#include "game/entity/native_entity.hpp"
#include "game/player/player.hpp"
#include "game/render/sat_pipeline_contract.hpp"

#include <cstdint>
#include <span>

namespace oasis::game::render {

struct TownSatFrameContext {
    std::int32_t camera_x{0};
    std::int32_t camera_y{0};
    std::uint16_t tick_counter{0};      // $FF1656 (used by 0x00A7E6 status bars)
    int hud_x_offset{0x20};             // $FF185A (0x20 right side, 0x138 left side)
    std::int32_t player_x{160};
    std::int32_t player_y{488};
    std::int16_t player_z{0};
    std::uint16_t player_facing{0};
    std::uint16_t player_frame_dir{0};  // +0x34 of Player Ali
    std::uint16_t player_vram_base{0x04A8U};
    std::uint16_t player_weapon_vram{0x04E8U};
    std::uint16_t player_overlay_3e{0xFFFFU};
    std::uint8_t player_flags36{0x01U};
    std::uint8_t player_shadow_idx{0x10U};
};

// Reconstructs hardware-exact Genesis VDP SAT entries directly from ROM sprite descriptor
// tables (0x13CDFC, 0x2AF1A6, 0x163716, 0x154B98, 0x00ACFE, 0x00AD56, 0x00A438) and
// Y-descending painter's algorithm (0x00A6C6..0x00A738). Zero prerecorded SAT bytes used.
void emit_rom_town_sat(SatPipelineContract& sat,
                       std::span<const std::uint8_t> rom,
                       const TownSatFrameContext& ctx,
                       std::span<const NativeEntity> entities) noexcept;

} // namespace oasis::game::render