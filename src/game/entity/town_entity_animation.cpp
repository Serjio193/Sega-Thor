#include "game/entity/town_entity_animation.hpp"

namespace oasis::game::entity {

namespace {

std::uint16_t read_be16(std::span<const std::uint8_t> d, std::size_t off) noexcept {
    if (off + 2U > d.size()) {
        return 0U;
    }
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(d[off]) << 8U) | d[off + 1U]);
}

std::int16_t read_be16s(std::span<const std::uint8_t> d, std::size_t off) noexcept {
    return static_cast<std::int16_t>(read_be16(d, off));
}

} // namespace

std::uint16_t step_prng(std::uint16_t& seed) noexcept {
    // ROM 0x00E268: 4 iterations of (bit = (d0 ^ rol16(d0, 2)) & 1; d0 = (d0 << 1) | bit)
    std::uint16_t d0 = seed;
    for (int i = 0; i < 4; ++i) {
        const std::uint16_t rol2 = static_cast<std::uint16_t>((d0 << 2U) | (d0 >> 14U));
        const std::uint16_t bit = static_cast<std::uint16_t>((d0 ^ rol2) & 1U);
        d0 = static_cast<std::uint16_t>((d0 << 1U) | bit);
    }
    seed = d0;
    return d0;
}

std::uint16_t prng_mod(std::uint16_t& seed, std::uint16_t bound) noexcept {
    // ROM 0x00E2A2: (step_prng(seed) * bound) >> 16
    const std::uint32_t r = step_prng(seed);
    return static_cast<std::uint16_t>((r * static_cast<std::uint32_t>(bound)) >> 16U);
}

void init_entity_animation(NativeEntity& ent, std::uint16_t anim_id) noexcept {
    // ROM 0x008EF8
    ent.anim_frame = static_cast<std::uint8_t>(anim_id & 0xFFU);
    ent.previous_anim_id = 0xFFFFU;
    ent.flags37 &= static_cast<std::uint8_t>(~0x06U);
    ent.anim_key_off = 0U;
    ent.anim_step = 0U;
}

void step_entity_animation(NativeEntity& ent,
                           std::span<const std::uint8_t> rom,
                           std::uint32_t anim_table_addr) noexcept {
    // ROM 0x008FA4..0x00903E
    if (rom.empty() || anim_table_addr + 2U > rom.size()) {
        return;
    }
    std::uint8_t d2 = ent.flags37;
    std::uint16_t d3 = ent.anim_step;
    const std::uint16_t d0_anim = ent.anim_frame;
    d2 &= static_cast<std::uint8_t>(~0x02U); // clear anim_ended bit 1

    if (ent.previous_anim_id != d0_anim) {
        ent.previous_anim_id = d0_anim;
        ent.anim_key_off = 0xFFFCU; // -4
        d3 = 0U;
        d2 &= static_cast<std::uint8_t>(~0x04U); // clear looped bit 2
    }

    const std::size_t tbl_entry = anim_table_addr + d0_anim;
    const std::int16_t rel = read_be16s(rom, tbl_entry);
    const std::size_t seq_base = static_cast<std::size_t>(static_cast<std::int32_t>(tbl_entry) + rel);

    const std::int16_t next_tmr = static_cast<std::int16_t>(d3) - 1;
    if (next_tmr < 0) {
        std::uint16_t off = static_cast<std::uint16_t>(ent.anim_key_off + 4U);
        d2 &= static_cast<std::uint8_t>(~0x08U);
        std::uint16_t frame = read_be16(rom, seq_base + off);
        if ((d2 & 0x04U) != 0U) {
            d2 &= static_cast<std::uint8_t>(~0x04U);
            off = 0U;
            frame = read_be16(rom, seq_base);
        }
        ent.frame_id = frame;
        ent.anim_key_off = off;
        std::uint16_t dur = read_be16(rom, seq_base + off + 2U);
        if ((dur & 0x8000U) != 0U) {
            dur &= 0x7FFFU;
            d2 |= 0x04U;
        }
        dur >>= 1U;
        if (dur > 0U) {
            --dur;
        }
        d3 = dur;
        if (d3 == 0U && (d2 & 0x04U) != 0U) {
            d2 |= 0x02U;
        }
    } else {
        d3 = static_cast<std::uint16_t>(next_tmr);
        if (d3 == 0U && (d2 & 0x04U) != 0U) {
            d2 |= 0x02U;
        }
    }

    ent.anim_step = d3;
    ent.flags37 = d2;
}

} // namespace oasis::game::entity