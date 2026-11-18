#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace oasis::game::dialogue {

/// Low-level Sega Genesis Sprite Attribute Table (SAT) piece entry (8 bytes).
struct SatPiece {
    std::int16_t y{0};
    std::uint16_t link_size{0};
    std::uint16_t tile{0};
    std::int16_t x{0};
};

/// High-level configuration parameters for dialogue sprite composition.
struct CompositorConfig {
    std::uint16_t plane_top_base{0U};
    std::uint16_t plane_bottom_base{0U};
    std::array<std::int16_t, 4> row_offsets{12, 24, 36, 48};
    std::int16_t line_counter{0};
    bool prompt_active{false};
    bool prompt_dismiss{false};
    bool anim_timer_visible{true};
};

/// State tracker for dialogue frame synchronization & render pacing (0x00C84E).
struct FrameSyncState {
    bool render_in_progress{false};
    std::uint16_t frame_sync_cadence{0U};
    std::uint8_t vint_status_flags{0U};
    std::uint32_t palette_ticks{0U};
    std::uint32_t render_passes{0U};
    std::uint32_t post_render_passes{0U};
};

/// Configuration and constants for ROM Dialogue Entity Sprite Compositor,
/// Prompt Arrow Indicator & Frame Sync Engine (0x00C756..0x00C88C, 310 bytes).
struct DialogueTaskCompositorConfig {
    static constexpr std::uint32_t kEntryTaskCompositorAddr = 0x00C756U;
    static constexpr std::uint32_t kEntryEmitLineSpritesAddr = 0x00C81CU;
    static constexpr std::uint32_t kEntryEmitSatPieceAddr = 0x00C838U;
    static constexpr std::uint32_t kEntryFrameSyncRenderAddr = 0x00C84EU;
    static constexpr std::uint32_t kRangeEndAddr = 0x00C88CU;
    static constexpr std::size_t kRoutineSizeBytes = 310U;

    // Prompt arrow indicator parameters
    static constexpr std::uint16_t kContinueArrowTile = 0x86DCU;
    static constexpr std::uint16_t kDismissArrowTile = 0x96DCU;
    static constexpr std::uint16_t kArrowTileDismissMask = 0x1000U; // bit 12
    static constexpr std::int16_t kArrowX = 284;                    // 0x011C
    static constexpr std::int16_t kArrowYBase = 332;                // 0x014C
    static constexpr std::int16_t kArrowYStep = 12;

    // Window line sprites grid
    static constexpr std::size_t kSpritesPerRow = 8U;
    static constexpr std::uint16_t kLineSpriteSizeCode = 0x0C00U; // 4x4 tiles (32x32 px)
    static constexpr std::int16_t kLineStartX = 160;              // 0x00A0
    static constexpr std::int16_t kLineStepX = 32;                // 0x0020
    static constexpr std::int16_t kLineYBias = 272;               // 0x0110
    static constexpr std::uint16_t kLineTileStep = 4U;

    // Sega Genesis hardware limits
    static constexpr std::uint16_t kHardwareSpriteLimit = 80U; // 0x50

    // Memory map addresses
    static constexpr std::uint32_t kSatBufferBaseRam = 0x00FF13CCU;
    static constexpr std::uint32_t kSatBufferOffsetRam = 0x00FF188CU;
    static constexpr std::uint32_t kSatLinkCounterRam = 0x00FF188AU;
    static constexpr std::uint32_t kPromptIconActiveRam = 0x00FF1886U;
    static constexpr std::uint32_t kPromptDismissFlagRam = 0x00FF1887U;
    static constexpr std::uint32_t kAnimationTimerRam = 0x00FF1657U;
    static constexpr std::uint32_t kLineCounterRam = 0x00FF1880U;
    static constexpr std::uint32_t kPlaneTopBaseRam = 0x00FF1872U;
    static constexpr std::uint32_t kPlaneBottomBaseRam = 0x00FF1874U;
    static constexpr std::uint32_t kLine1RowOffsetRam = 0x00FF1876U;
    static constexpr std::uint32_t kLine2RowOffsetRam = 0x00FF1878U;
    static constexpr std::uint32_t kLine3RowOffsetRam = 0x00FF187AU;
    static constexpr std::uint32_t kLine4RowOffsetRam = 0x00FF187CU;
    static constexpr std::uint32_t kRenderInProgressFlagRam = 0x00FF185DU;
    static constexpr std::uint32_t kFrameSyncCadenceRam = 0x00FF1654U;
    static constexpr std::uint32_t kVintStatusFlagsRam = 0x00FF164EU;
};

/// Computes dynamic prompt arrow vertical Y coordinate based on active line counter.
[[nodiscard]] std::int16_t calculate_prompt_arrow_y(std::int16_t line_counter) noexcept;

/// Appends a single SAT piece if within 80-sprite hardware limit (0x00C838).
bool emit_sat_piece(
    std::vector<SatPiece>& pieces,
    std::int16_t y,
    std::uint16_t size_code,
    std::uint16_t tile,
    std::int16_t x,
    std::uint16_t& io_link);

/// Emits 8 sprite pieces of 4x4 tiles across an 8-tile text row (0x00C81C).
void emit_line_sprites(
    std::vector<SatPiece>& pieces,
    std::int16_t row_offset,
    std::uint16_t base_tile,
    std::uint16_t& io_link);

/// Pure cleanroom compositor for prompt arrow and 4 dialogue window lines (0x00C756).
[[nodiscard]] std::vector<SatPiece> composite_dialogue_sprites(
    const CompositorConfig& config,
    std::uint16_t& io_link);

/// Steps dialogue frame synchronization, cadence management, and render pass (0x00C84E).
void step_frame_sync_and_render(
    FrameSyncState& state,
    const std::function<void()>& palette_tick = {},
    const std::function<void()>& render_pass = {},
    const std::function<void()>& post_render_pass = {});

/// Raw 64KB RAM window integration helper for sprite composition.
void composite_dialogue_sprites_raw(std::span<std::uint8_t> ram);

/// Raw 64KB RAM window integration helper for frame sync & render pass.
void step_frame_sync_raw(
    std::span<std::uint8_t> ram,
    const std::function<void()>& palette_tick = {},
    const std::function<void()>& render_pass = {},
    const std::function<void()>& post_render_pass = {});

} // namespace oasis::game::dialogue

namespace thor::dialogue {
using SatPiece = oasis::game::dialogue::SatPiece;
using CompositorConfig = oasis::game::dialogue::CompositorConfig;
using FrameSyncState = oasis::game::dialogue::FrameSyncState;
using DialogueTaskCompositorConfig = oasis::game::dialogue::DialogueTaskCompositorConfig;
using oasis::game::dialogue::calculate_prompt_arrow_y;
using oasis::game::dialogue::composite_dialogue_sprites;
using oasis::game::dialogue::emit_line_sprites;
using oasis::game::dialogue::emit_sat_piece;
using oasis::game::dialogue::step_frame_sync_and_render;
} // namespace thor::dialogue
