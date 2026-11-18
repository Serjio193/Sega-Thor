#include "game/render/scroll_integrator.hpp"

namespace thor::render {

namespace {

[[nodiscard]] inline std::uint16_t read_be16(
    std::span<const std::uint8_t, 0x10000> data,
    std::size_t offset) noexcept
{
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(data[offset]) << 8) | data[offset + 1]);
}

inline void write_be16(
    std::span<std::uint8_t, 0x10000> data,
    std::size_t offset,
    std::uint16_t value) noexcept
{
    data[offset] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
    data[offset + 1] = static_cast<std::uint8_t>(value & 0xFF);
}

} // namespace

void accumulate_long_and_integrate(
    TilemapCameraState& state,
    std::int32_t delta_long_x,
    std::int32_t delta_long_y) noexcept
{
    // 0x00D146: btst.b #0, $27(a0); beq.w $d264
    if ((state.control_flags & kControlEnableIntegration) == 0) {
        return;
    }

    // 0x00D150: add.l $0(a0), d1
    const std::uint32_t cur_x = (static_cast<std::uint32_t>(static_cast<std::uint16_t>(state.cam_x)) << 16) | state.cam_sub_x;
    const std::uint32_t res_x = cur_x + static_cast<std::uint32_t>(delta_long_x);
    state.cam_sub_x = static_cast<std::uint16_t>(res_x & 0xFFFF);
    const std::int16_t new_x = static_cast<std::int16_t>(static_cast<std::uint16_t>(res_x >> 16));

    // 0x00D154: add.l $4(a0), d2
    const std::uint32_t cur_y = (static_cast<std::uint32_t>(static_cast<std::uint16_t>(state.cam_y)) << 16) | state.cam_sub_y;
    const std::uint32_t res_y = cur_y + static_cast<std::uint32_t>(delta_long_y);
    state.cam_sub_y = static_cast<std::uint16_t>(res_y & 0xFFFF);
    const std::int16_t new_y = static_cast<std::int16_t>(static_cast<std::uint16_t>(res_y >> 16));

    // 0x00D164: bra.w $d188
    integrate_scroll_deltas(state, new_x, new_y);
}

void accumulate_word_and_integrate(
    TilemapCameraState& state,
    std::int16_t delta_word_x,
    std::int16_t delta_word_y) noexcept
{
    // 0x00D168: btst.b #0, $27(a0); beq.w $d264
    if ((state.control_flags & kControlEnableIntegration) == 0) {
        return;
    }

    // 0x00D172: add.w $0(a0), d1; add.w $4(a0), d2
    const std::int16_t new_x = static_cast<std::int16_t>(state.cam_x + delta_word_x);
    const std::int16_t new_y = static_cast<std::int16_t>(state.cam_y + delta_word_y);

    // 0x00D17A: bra.w $d188
    integrate_scroll_deltas(state, new_x, new_y);
}

bool check_enable_and_integrate(
    TilemapCameraState& state,
    std::int16_t new_x,
    std::int16_t new_y) noexcept
{
    // 0x00D17E: btst.b #0, $27(a0); beq.w $d264
    if ((state.control_flags & kControlEnableIntegration) == 0) {
        return false;
    }
    integrate_scroll_deltas(state, new_x, new_y);
    return true;
}

void integrate_scroll_deltas(
    TilemapCameraState& state,
    std::int16_t new_x,
    std::int16_t new_y) noexcept
{
    // 0x00D188: clr.w d0; move.w d0, $12(a0); move.w d0, $14(a0)
    std::uint8_t dirty = 0;
    state.delta_x = 0;
    state.delta_y = 0;

    const std::int16_t old_x = state.cam_x;
    if (new_x != old_x) {
        // 0x00D19C: move.w d1, $0(a0)
        state.cam_x = new_x;
        const std::int16_t delta_x = static_cast<std::int16_t>(new_x - old_x);

        // 0x00D1A4: btst.b #1, $27(a0)
        if ((state.control_flags & kControlSuppressTileStripShift) == 0) {
            const std::int16_t old_col = static_cast<std::int16_t>(old_x >> 3);
            const std::int16_t new_col = static_cast<std::int16_t>(new_x >> 3);
            const std::int16_t col_step = static_cast<std::int16_t>(new_col - old_col);
            if (col_step != 0) {
                state.tile_col_step = col_step;
                dirty |= kDirtyBitTileColStrip; // 0x01
            }
        }

        // 0x00D1BE: move.w d5, $12(a0)
        state.delta_x = delta_x;

        // 0x00D1C2: neg.w d5; add.w $e(a0), d5; andi.w #$1ff, d5; move.w d5, $e(a0)
        state.vdp_hscroll = static_cast<std::uint16_t>((state.vdp_hscroll - delta_x) & kHScrollMask);
        dirty |= kDirtyBitPlaneAHScroll; // 0x04

        // 0x00D1D4..0x00D1F6: +/-10000 range check guard
        const std::int16_t map_width_px = static_cast<std::int16_t>(state.map_width_tiles << 3);
        if (state.cam_x <= -10000) {
            state.cam_x = static_cast<std::int16_t>(state.cam_x + map_width_px + map_width_px);
        }
        if (state.cam_x >= 10000) {
            state.cam_x = static_cast<std::int16_t>(state.cam_x - map_width_px - map_width_px);
        }
    }

    const std::int16_t old_y = state.cam_y;
    if (new_y != old_y) {
        // 0x00D204: move.w d2, $4(a0)
        state.cam_y = new_y;
        const std::int16_t delta_y = static_cast<std::int16_t>(new_y - old_y);

        // 0x00D20C: btst.b #1, $27(a0)
        if ((state.control_flags & kControlSuppressTileStripShift) == 0) {
            const std::int16_t old_row = static_cast<std::int16_t>(old_y >> 3);
            const std::int16_t new_row = static_cast<std::int16_t>(new_y >> 3);
            const std::int16_t row_step = static_cast<std::int16_t>(new_row - old_row);
            if (row_step != 0) {
                state.tile_row_step = row_step;
                dirty |= kDirtyBitTileRowStrip; // 0x02
            }
        }

        // 0x00D226: move.w d6, $14(a0)
        state.delta_y = delta_y;

        // 0x00D22A: add.w $10(a0), d6; andi.w #$ff, d6; move.w d6, $10(a0)
        state.vdp_vscroll = static_cast<std::uint16_t>((state.vdp_vscroll + delta_y) & kVScrollMask);
        dirty |= kDirtyBitPlaneAVScroll; // 0x08

        // 0x00D23A..0x00D25C: +/-10000 range check guard
        const std::int16_t map_height_px = static_cast<std::int16_t>(state.map_height_tiles << 3);
        if (state.cam_y <= -10000) {
            state.cam_y = static_cast<std::int16_t>(state.cam_y + map_height_px + map_height_px);
        }
        if (state.cam_y >= 10000) {
            state.cam_y = static_cast<std::int16_t>(state.cam_y - map_height_px - map_height_px);
        }
    }

    // 0x00D260: move.b d0, $26(a0)
    state.dirty_flags = dirty;
}

void normalize_toroidal_coords(
    TilemapCameraState& state) noexcept
{
    // 0x00D266..0x00D288: normalize X
    const std::int16_t map_width_px = static_cast<std::int16_t>(state.map_width_tiles << 3);
    if (map_width_px > 0) {
        while (state.cam_x < 0) {
            state.cam_x = static_cast<std::int16_t>(state.cam_x + map_width_px);
        }
        while (static_cast<std::uint16_t>(state.cam_x) >= static_cast<std::uint16_t>(map_width_px)) {
            state.cam_x = static_cast<std::int16_t>(state.cam_x - map_width_px);
        }
    }

    // 0x00D28C..0x00D2AE: normalize Y
    const std::int16_t map_height_px = static_cast<std::int16_t>(state.map_height_tiles << 3);
    if (map_height_px > 0) {
        while (state.cam_y < 0) {
            state.cam_y = static_cast<std::int16_t>(state.cam_y + map_height_px);
        }
        while (static_cast<std::uint16_t>(state.cam_y) >= static_cast<std::uint16_t>(map_height_px)) {
            state.cam_y = static_cast<std::int16_t>(state.cam_y - map_height_px);
        }
    }
}

TilemapCameraState load_state_from_ram(
    std::span<const std::uint8_t, 0x10000> ram,
    std::uint32_t plane_ram_offset) noexcept
{
    TilemapCameraState state{};
    state.cam_x = static_cast<std::int16_t>(read_be16(ram, plane_ram_offset + 0x00));
    state.cam_sub_x = read_be16(ram, plane_ram_offset + 0x02);
    state.cam_y = static_cast<std::int16_t>(read_be16(ram, plane_ram_offset + 0x04));
    state.cam_sub_y = read_be16(ram, plane_ram_offset + 0x06);
    state.vdp_hscroll = read_be16(ram, plane_ram_offset + 0x0E);
    state.vdp_vscroll = read_be16(ram, plane_ram_offset + 0x10);
    state.delta_x = static_cast<std::int16_t>(read_be16(ram, plane_ram_offset + 0x12));
    state.delta_y = static_cast<std::int16_t>(read_be16(ram, plane_ram_offset + 0x14));
    state.tile_col_step = static_cast<std::int16_t>(read_be16(ram, plane_ram_offset + 0x1C));
    state.tile_row_step = static_cast<std::int16_t>(read_be16(ram, plane_ram_offset + 0x1E));
    state.map_width_tiles = read_be16(ram, plane_ram_offset + 0x20);
    state.map_height_tiles = read_be16(ram, plane_ram_offset + 0x22);
    state.dirty_flags = ram[plane_ram_offset + 0x26];
    state.control_flags = ram[plane_ram_offset + 0x27];
    return state;
}

void store_state_to_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t plane_ram_offset,
    const TilemapCameraState& state) noexcept
{
    write_be16(ram, plane_ram_offset + 0x00, static_cast<std::uint16_t>(state.cam_x));
    write_be16(ram, plane_ram_offset + 0x02, state.cam_sub_x);
    write_be16(ram, plane_ram_offset + 0x04, static_cast<std::uint16_t>(state.cam_y));
    write_be16(ram, plane_ram_offset + 0x06, state.cam_sub_y);
    write_be16(ram, plane_ram_offset + 0x0E, state.vdp_hscroll);
    write_be16(ram, plane_ram_offset + 0x10, state.vdp_vscroll);
    write_be16(ram, plane_ram_offset + 0x12, static_cast<std::uint16_t>(state.delta_x));
    write_be16(ram, plane_ram_offset + 0x14, static_cast<std::uint16_t>(state.delta_y));
    write_be16(ram, plane_ram_offset + 0x1C, static_cast<std::uint16_t>(state.tile_col_step));
    write_be16(ram, plane_ram_offset + 0x1E, static_cast<std::uint16_t>(state.tile_row_step));
    write_be16(ram, plane_ram_offset + 0x20, state.map_width_tiles);
    write_be16(ram, plane_ram_offset + 0x22, state.map_height_tiles);
    ram[plane_ram_offset + 0x26] = state.dirty_flags;
    ram[plane_ram_offset + 0x27] = state.control_flags;
}

void accumulate_long_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t plane_ram_offset,
    std::int32_t delta_long_x,
    std::int32_t delta_long_y) noexcept
{
    TilemapCameraState state = load_state_from_ram(ram, plane_ram_offset);
    accumulate_long_and_integrate(state, delta_long_x, delta_long_y);
    store_state_to_ram(ram, plane_ram_offset, state);
}

void accumulate_word_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t plane_ram_offset,
    std::int16_t delta_word_x,
    std::int16_t delta_word_y) noexcept
{
    TilemapCameraState state = load_state_from_ram(ram, plane_ram_offset);
    accumulate_word_and_integrate(state, delta_word_x, delta_word_y);
    store_state_to_ram(ram, plane_ram_offset, state);
}

void integrate_scroll_deltas_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t plane_ram_offset,
    std::int16_t new_x,
    std::int16_t new_y) noexcept
{
    TilemapCameraState state = load_state_from_ram(ram, plane_ram_offset);
    integrate_scroll_deltas(state, new_x, new_y);
    store_state_to_ram(ram, plane_ram_offset, state);
}

void normalize_toroidal_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t plane_ram_offset) noexcept
{
    TilemapCameraState state = load_state_from_ram(ram, plane_ram_offset);
    normalize_toroidal_coords(state);
    store_state_to_ram(ram, plane_ram_offset, state);
}

} // namespace thor::render
