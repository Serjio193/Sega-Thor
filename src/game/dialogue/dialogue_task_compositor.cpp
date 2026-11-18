#include "dialogue_task_compositor.hpp"

namespace oasis::game::dialogue {

namespace {

inline std::uint16_t read_be16(std::span<const std::uint8_t> ram, std::uint32_t addr) noexcept {
    const std::size_t off = static_cast<std::size_t>(addr & 0xFFFFU);
    if (off + 1U >= ram.size()) {
        return 0U;
    }
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(ram[off]) << 8) |
                                      static_cast<std::uint16_t>(ram[off + 1U]));
}

inline void write_be16(std::span<std::uint8_t> ram, std::uint32_t addr, std::uint16_t val) noexcept {
    const std::size_t off = static_cast<std::size_t>(addr & 0xFFFFU);
    if (off + 1U >= ram.size()) {
        return;
    }
    ram[off] = static_cast<std::uint8_t>((val >> 8) & 0xFFU);
    ram[off + 1U] = static_cast<std::uint8_t>(val & 0xFFU);
}

inline std::uint8_t read_u8(std::span<const std::uint8_t> ram, std::uint32_t addr) noexcept {
    const std::size_t off = static_cast<std::size_t>(addr & 0xFFFFU);
    if (off >= ram.size()) {
        return 0U;
    }
    return ram[off];
}

inline void write_u8(std::span<std::uint8_t> ram, std::uint32_t addr, std::uint8_t val) noexcept {
    const std::size_t off = static_cast<std::size_t>(addr & 0xFFFFU);
    if (off >= ram.size()) {
        return;
    }
    ram[off] = val;
}

} // namespace

std::int16_t calculate_prompt_arrow_y(std::int16_t line_counter) noexcept {
    std::int16_t arrow_y = DialogueTaskCompositorConfig::kArrowYBase;
    if (line_counter < 3) {
        arrow_y = static_cast<std::int16_t>(arrow_y - DialogueTaskCompositorConfig::kArrowYStep);
        if (line_counter < 2) {
            arrow_y = static_cast<std::int16_t>(arrow_y - DialogueTaskCompositorConfig::kArrowYStep);
            if (line_counter < 1) {
                arrow_y = static_cast<std::int16_t>(arrow_y - DialogueTaskCompositorConfig::kArrowYStep);
            }
        }
    }
    return arrow_y;
}

bool emit_sat_piece(
    std::vector<SatPiece>& pieces,
    std::int16_t y,
    std::uint16_t size_code,
    std::uint16_t tile,
    std::int16_t x,
    std::uint16_t& io_link) {
    if (io_link >= DialogueTaskCompositorConfig::kHardwareSpriteLimit) {
        return false;
    }
    ++io_link;
    pieces.push_back(SatPiece{
        .y = y,
        .link_size = static_cast<std::uint16_t>(size_code + io_link),
        .tile = tile,
        .x = x,
    });
    return true;
}

void emit_line_sprites(
    std::vector<SatPiece>& pieces,
    std::int16_t row_offset,
    std::uint16_t base_tile,
    std::uint16_t& io_link) {
    std::int16_t y = static_cast<std::int16_t>(row_offset + DialogueTaskCompositorConfig::kLineYBias);
    std::int16_t x = DialogueTaskCompositorConfig::kLineStartX;
    std::uint16_t tile = base_tile;

    for (std::size_t i = 0; i < DialogueTaskCompositorConfig::kSpritesPerRow; ++i) {
        if (!emit_sat_piece(pieces, y, DialogueTaskCompositorConfig::kLineSpriteSizeCode, tile, x, io_link)) {
            break;
        }
        x = static_cast<std::int16_t>(x + DialogueTaskCompositorConfig::kLineStepX);
        tile = static_cast<std::uint16_t>(tile + DialogueTaskCompositorConfig::kLineTileStep);
    }
}

std::vector<SatPiece> composite_dialogue_sprites(
    const CompositorConfig& config,
    std::uint16_t& io_link) {
    std::vector<SatPiece> pieces;
    pieces.reserve(33U);

    if (config.prompt_active && config.anim_timer_visible) {
        std::uint16_t tile = DialogueTaskCompositorConfig::kContinueArrowTile;
        if (config.prompt_dismiss) {
            tile = static_cast<std::uint16_t>(tile | DialogueTaskCompositorConfig::kArrowTileDismissMask);
        }
        const std::int16_t y = calculate_prompt_arrow_y(config.line_counter);
        emit_sat_piece(pieces, y, 0U, tile, DialogueTaskCompositorConfig::kArrowX, io_link);
    }

    const std::uint16_t top_base_tile = static_cast<std::uint16_t>(
        (config.plane_top_base >> 5) | 0x8000U);
    emit_line_sprites(pieces, config.row_offsets[0], top_base_tile, io_link);
    emit_line_sprites(pieces, config.row_offsets[1], top_base_tile, io_link);

    const std::uint16_t bottom_base_tile = static_cast<std::uint16_t>(
        (config.plane_bottom_base >> 5) | 0x8000U);
    emit_line_sprites(pieces, config.row_offsets[2], bottom_base_tile, io_link);
    emit_line_sprites(pieces, config.row_offsets[3], bottom_base_tile, io_link);

    return pieces;
}

void step_frame_sync_and_render(
    FrameSyncState& state,
    const std::function<void()>& palette_tick,
    const std::function<void()>& render_pass,
    const std::function<void()>& post_render_pass) {
    state.render_in_progress = true;

    if (state.frame_sync_cadence == 0U) {
        state.frame_sync_cadence = 2U;
        ++state.palette_ticks;
        if (palette_tick) {
            palette_tick();
        }
        ++state.render_passes;
        if (render_pass) {
            render_pass();
        }
        state.render_in_progress = false;
    }

    if ((state.vint_status_flags & 0x01U) == 0U) {
        ++state.post_render_passes;
        if (post_render_pass) {
            post_render_pass();
        }
        state.render_in_progress = false;
    }
}

void composite_dialogue_sprites_raw(std::span<std::uint8_t> ram) {
    if (ram.size() < 0x10000U) {
        return;
    }

    CompositorConfig config;
    config.plane_top_base = read_be16(ram, DialogueTaskCompositorConfig::kPlaneTopBaseRam);
    config.plane_bottom_base = read_be16(ram, DialogueTaskCompositorConfig::kPlaneBottomBaseRam);
    config.row_offsets[0] = static_cast<std::int16_t>(read_be16(ram, DialogueTaskCompositorConfig::kLine1RowOffsetRam));
    config.row_offsets[1] = static_cast<std::int16_t>(read_be16(ram, DialogueTaskCompositorConfig::kLine2RowOffsetRam));
    config.row_offsets[2] = static_cast<std::int16_t>(read_be16(ram, DialogueTaskCompositorConfig::kLine3RowOffsetRam));
    config.row_offsets[3] = static_cast<std::int16_t>(read_be16(ram, DialogueTaskCompositorConfig::kLine4RowOffsetRam));
    config.line_counter = static_cast<std::int16_t>(read_be16(ram, DialogueTaskCompositorConfig::kLineCounterRam));

    config.prompt_active = (read_u8(ram, DialogueTaskCompositorConfig::kPromptIconActiveRam) != 0U);
    config.prompt_dismiss = (read_u8(ram, DialogueTaskCompositorConfig::kPromptDismissFlagRam) != 0U);
    config.anim_timer_visible = ((read_u8(ram, DialogueTaskCompositorConfig::kAnimationTimerRam) & 0x10U) != 0U);

    std::uint16_t link = read_be16(ram, DialogueTaskCompositorConfig::kSatLinkCounterRam);
    const std::vector<SatPiece> pieces = composite_dialogue_sprites(config, link);

    std::uint16_t sat_offset = read_be16(ram, DialogueTaskCompositorConfig::kSatBufferOffsetRam);
    std::uint32_t sat_write_addr = DialogueTaskCompositorConfig::kSatBufferBaseRam + sat_offset;

    for (const auto& piece : pieces) {
        if ((sat_write_addr & 0xFFFFU) + 8U > ram.size()) {
            break;
        }
        write_be16(ram, sat_write_addr, static_cast<std::uint16_t>(piece.y));
        write_be16(ram, sat_write_addr + 2U, piece.link_size);
        write_be16(ram, sat_write_addr + 4U, piece.tile);
        write_be16(ram, sat_write_addr + 6U, static_cast<std::uint16_t>(piece.x));

        sat_write_addr += 8U;
        sat_offset = static_cast<std::uint16_t>(sat_offset + 8U);
    }

    write_be16(ram, DialogueTaskCompositorConfig::kSatBufferOffsetRam, sat_offset);
    write_be16(ram, DialogueTaskCompositorConfig::kSatLinkCounterRam, link);
}

void step_frame_sync_raw(
    std::span<std::uint8_t> ram,
    const std::function<void()>& palette_tick,
    const std::function<void()>& render_pass,
    const std::function<void()>& post_render_pass) {
    if (ram.size() < 0x10000U) {
        return;
    }

    FrameSyncState state;
    state.render_in_progress = (read_u8(ram, DialogueTaskCompositorConfig::kRenderInProgressFlagRam) != 0U);
    state.frame_sync_cadence = read_be16(ram, DialogueTaskCompositorConfig::kFrameSyncCadenceRam);
    state.vint_status_flags = read_u8(ram, DialogueTaskCompositorConfig::kVintStatusFlagsRam);

    step_frame_sync_and_render(state, palette_tick, render_pass, post_render_pass);

    write_u8(ram, DialogueTaskCompositorConfig::kRenderInProgressFlagRam,
             state.render_in_progress ? 0xFFU : 0x00U);
    write_be16(ram, DialogueTaskCompositorConfig::kFrameSyncCadenceRam, state.frame_sync_cadence);
}

} // namespace oasis::game::dialogue
