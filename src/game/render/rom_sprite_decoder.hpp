#pragma once

#include "game/entity/native_entity.hpp"
#include "game/player/player.hpp"
#include "game/render/sat_pipeline_contract.hpp"
#include "game/render/vdp_dma_queue.hpp"
#include "game/spirits/dytto_controller.hpp"

#include <cstdint>
#include <span>

namespace oasis::game::render {

struct TownEffect {
    bool active{false};
    std::int32_t world_x{0};
    std::int32_t world_y{0};
    std::uint8_t timer{0};
};

struct TownSatFrameContext {
    std::int32_t camera_x{0};
    std::int32_t camera_y{0};
    std::uint16_t tick_counter{0};      // $FF1656 (used by 0x00A7E6 status bars)
    std::uint32_t frame_60hz{0};        // $FF1971 (60Hz VBLANK frame counter for 0x00AB1A shake)
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
    bool attract_banner_enabled{false}; // ROM 0x00A7E6: suppressed when $FF1996 == 0 (free controller gameplay)
    std::array<TownEffect, 4> effect_pool_0{};
};

// Reconstructs hardware-exact Genesis VDP SAT entries directly from ROM sprite descriptor
// tables (0x13CDFC, 0x2AF1A6, 0x163716, 0x154B98, 0x00ACFE, 0x00AD56, 0x00A438) and
// Y-descending painter's algorithm (0x00A6C6..0x00A738). Zero prerecorded SAT bytes used.
void emit_rom_town_sat(SatPipelineContract& sat,
                       std::span<const std::uint8_t> rom,
                       const TownSatFrameContext& ctx,
                       std::span<const NativeEntity> entities,
                       VdpDmaQueue* dma_queue = nullptr,
                       const spirits::DyttoSpiritController* dytto = nullptr) noexcept;

} // namespace oasis::game::render