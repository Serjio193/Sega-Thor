#include "game/player/anim_palette_entity_stepper.hpp"

namespace oasis::game::player {

namespace {

inline void write_u16_be(std::span<std::uint8_t> buf, std::size_t offset, std::uint16_t val) noexcept {
    if (offset + 2U <= buf.size()) {
        buf[offset] = static_cast<std::uint8_t>((val >> 8U) & 0xFFU);
        buf[offset + 1U] = static_cast<std::uint8_t>(val & 0xFFU);
    }
}

inline std::uint16_t read_u16_be(std::span<const std::uint8_t> buf, std::size_t offset) noexcept {
    if (offset + 2U <= buf.size()) {
        return static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(buf[offset]) << 8U) |
            static_cast<std::uint16_t>(buf[offset + 1U]));
    }
    return 0U;
}

} // namespace

bool step_anim_palette_raw(
    bool active,
    std::span<std::uint8_t> table,
    std::span<std::uint8_t> cram_buffer) noexcept {
    if (!active || table.size() < 8U) {
        return false;
    }

    bool any_changed = false;
    std::size_t entry_offset = 0U;

    while (entry_offset + 8U <= table.size()) {
        std::uint16_t timer = read_u16_be(table, entry_offset + 0x06U);
        bool advance = false;

        // 0x008E3E..0x008E46: subq.w #1, d2; beq $8e4a; subq.w #1, d2; bne $8e70
        if (timer <= 2U) {
            advance = true;
        } else {
            timer = static_cast<std::uint16_t>(timer - 2U);
            write_u16_be(table, entry_offset + 0x06U, timer);
        }

        if (advance) {
            // 0x008E4A..0x008E6E: advance to next frame
            std::uint16_t frame_offset = read_u16_be(table, entry_offset + 0x04U);
            frame_offset = static_cast<std::uint16_t>(frame_offset + 4U);

            if (entry_offset + frame_offset + 2U > table.size()) {
                break;
            }

            std::uint16_t duration = read_u16_be(table, entry_offset + frame_offset);
            if (duration == 0U) {
                // Loop back: first frame is at offset 8
                frame_offset = 8U;
                if (entry_offset + frame_offset + 2U > table.size()) {
                    break;
                }
                duration = read_u16_be(table, entry_offset + frame_offset);
            }

            // Update active frame offset
            write_u16_be(table, entry_offset + 0x04U, frame_offset);

            // Read target CRAM byte offset from +0x02
            std::uint16_t cram_off = read_u16_be(table, entry_offset + 0x02U);
            // Read 16-bit color at frame_offset + 2
            std::uint16_t color = read_u16_be(table, entry_offset + frame_offset + 2U);

            if (cram_off + 2U <= cram_buffer.size()) {
                write_u16_be(cram_buffer, cram_off, color);
            }

            any_changed = true;
            // Set new frame duration in timer at +0x06
            write_u16_be(table, entry_offset + 0x06U, duration);
        }

        // Check stride to next animation entry at +0x00
        std::uint16_t stride = read_u16_be(table, entry_offset + 0x00U);
        if (stride == 0U) {
            break;
        }
        entry_offset += stride;
    }

    return any_changed;
}

std::vector<std::uint8_t> create_anim_palette_entry(
    std::uint16_t target_cram_offset,
    std::uint16_t initial_frame_offset,
    std::uint16_t initial_timer,
    std::span<const AnimPaletteFrame> frames,
    std::uint16_t stride_to_next) {
    std::size_t total_size = 8U + (frames.size() + 1U) * 4U;
    std::vector<std::uint8_t> buf(total_size, 0U);

    write_u16_be(buf, 0x00U, stride_to_next);
    write_u16_be(buf, 0x02U, target_cram_offset);
    write_u16_be(buf, 0x04U, initial_frame_offset);
    write_u16_be(buf, 0x06U, initial_timer);

    std::size_t off = 8U;
    for (const auto& frame : frames) {
        write_u16_be(buf, off, frame.duration);
        write_u16_be(buf, off + 2U, frame.color);
        off += 4U;
    }
    // Sentinel frame
    write_u16_be(buf, off, 0U);
    write_u16_be(buf, off + 2U, 0U);

    return buf;
}

void reset_entity_state(
    std::span<std::uint8_t> slot,
    std::uint16_t new_state) noexcept {
    if (slot.size() >= 0x38U) {
        write_u16_be(slot, AnimPaletteEntityStepperConfig::kStateOffset, new_state);
        write_u16_be(slot, AnimPaletteEntityStepperConfig::kTimerOffset, AnimPaletteEntityStepperConfig::kTimerInitVal);
        slot[AnimPaletteEntityStepperConfig::kFlags37Offset] &= AnimPaletteEntityStepperConfig::kFlags37Mask;
        write_u16_be(slot, AnimPaletteEntityStepperConfig::kSubtimer30Offset, 0U);
        write_u16_be(slot, AnimPaletteEntityStepperConfig::kSubtimer2eOffset, 0U);
    }
}

void dispatch_secondary_entity(
    std::span<std::uint8_t> slot) noexcept {
    if (slot.size() > AnimPaletteEntityStepperConfig::kFlags37Offset) {
        slot[AnimPaletteEntityStepperConfig::kFlags37Offset] |= static_cast<std::uint8_t>(1U << AnimPaletteEntityStepperConfig::kSecondaryActiveBit);
    }
}

EntityMasterStepperStats step_entity_groups_raw(
    std::span<std::uint8_t> group1_mem,
    std::span<std::uint8_t> group2_mem,
    std::span<std::uint8_t> group3_mem,
    std::uint32_t& recovery_vector_ram,
    const SecondaryEntityHandler& on_secondary,
    const PrimaryEntityHandler& on_primary) noexcept {
    EntityMasterStepperStats stats{};

    // Group 1: 4 slots x 90 bytes at $FF2954
    recovery_vector_ram = AnimPaletteEntityStepperConfig::kGroup1RecoveryVector;
    for (std::size_t i = 0U; i < AnimPaletteEntityStepperConfig::kGroup1Slots; ++i) {
        const std::size_t off = i * AnimPaletteEntityStepperConfig::kGroup1Stride;
        if (off + AnimPaletteEntityStepperConfig::kGroup1Stride <= group1_mem.size()) {
            auto slot = group1_mem.subspan(off, AnimPaletteEntityStepperConfig::kGroup1Stride);
            auto type = static_cast<std::int16_t>(read_u16_be(slot, 0x00U));
            if (type > 0) {
                dispatch_secondary_entity(slot);
                if (on_secondary) {
                    on_secondary(0U, i, slot);
                }
                ++stats.group1_active_count;
            }
        }
    }

    // Group 2: 21 slots x 188 bytes at $FF19E8
    recovery_vector_ram = AnimPaletteEntityStepperConfig::kGroup2RecoveryVector;
    for (std::size_t i = 0U; i < AnimPaletteEntityStepperConfig::kGroup2Slots; ++i) {
        const std::size_t off = i * AnimPaletteEntityStepperConfig::kGroup2Stride;
        if (off + AnimPaletteEntityStepperConfig::kGroup2Stride <= group2_mem.size()) {
            auto slot = group2_mem.subspan(off, AnimPaletteEntityStepperConfig::kGroup2Stride);
            auto type = static_cast<std::int16_t>(read_u16_be(slot, 0x00U));
            if (type > 0) {
                if (on_primary) {
                    on_primary(i, slot);
                }
                ++stats.group2_active_count;
            }
        }
    }

    // Group 3: 6 slots x 90 bytes at $FF2D8C
    recovery_vector_ram = AnimPaletteEntityStepperConfig::kGroup3RecoveryVector;
    for (std::size_t i = 0U; i < AnimPaletteEntityStepperConfig::kGroup3Slots; ++i) {
        const std::size_t off = i * AnimPaletteEntityStepperConfig::kGroup3Stride;
        if (off + AnimPaletteEntityStepperConfig::kGroup3Stride <= group3_mem.size()) {
            auto slot = group3_mem.subspan(off, AnimPaletteEntityStepperConfig::kGroup3Stride);
            auto type = static_cast<std::int16_t>(read_u16_be(slot, 0x00U));
            if (type > 0) {
                dispatch_secondary_entity(slot);
                if (on_secondary) {
                    on_secondary(2U, i, slot);
                }
                ++stats.group3_active_count;
            }
        }
    }

    stats.last_recovery_addr = recovery_vector_ram;
    return stats;
}

} // namespace oasis::game::player
