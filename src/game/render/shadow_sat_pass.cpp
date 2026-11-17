#include "game/render/shadow_sat_pass.hpp"

#include <algorithm>

namespace oasis::game::render {

namespace {

[[nodiscard]] inline std::int16_t read_i16_be(std::span<const std::uint8_t> buf, std::size_t offset) noexcept {
    if (offset + 1 >= buf.size()) {
        return 0;
    }
    return static_cast<std::int16_t>(
        (static_cast<std::uint16_t>(buf[offset]) << 8) |
        static_cast<std::uint16_t>(buf[offset + 1]));
}

[[nodiscard]] inline std::uint16_t read_u16_be(std::span<const std::uint8_t> buf, std::size_t offset) noexcept {
    if (offset + 1 >= buf.size()) {
        return 0U;
    }
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(buf[offset]) << 8) |
        static_cast<std::uint16_t>(buf[offset + 1]));
}

} // namespace

std::uint32_t build_shadow_inactive_mask(
    std::span<const ShadowEntitySlot> group2) noexcept {
    std::uint32_t mask = 0U;
    const std::size_t count = std::min(group2.size(), ShadowSatPassConfig::kGroup2SlotCount);
    for (std::size_t i = 0U; i < count; ++i) {
        if (!group2[i].has_shadow()) {
            mask |= (1U << i);
        }
    }
    for (std::size_t i = count; i < ShadowSatPassConfig::kGroup2SlotCount; ++i) {
        mask |= (1U << i);
    }
    return mask;
}

std::vector<std::size_t> sort_group2_shadows_y_depth_order(
    std::span<const ShadowEntitySlot> group2,
    std::uint32_t* out_done_mask) {
    std::vector<std::size_t> order;
    std::uint32_t done_mask = build_shadow_inactive_mask(group2);
    const std::size_t count = std::min(group2.size(), ShadowSatPassConfig::kGroup2SlotCount);

    while (true) {
        std::int16_t max_y = ShadowSatPassConfig::kInitialMaxYSentinel;
        int best_slot = -1;

        // ROM 0x00A6EE..0x00A712 scans from d6 = 20 down to 0
        for (int d6 = static_cast<int>(ShadowSatPassConfig::kGroup2SlotCount) - 1; d6 >= 0; --d6) {
            if ((done_mask & (1U << static_cast<std::uint32_t>(d6))) != 0U) {
                continue;
            }
            if (static_cast<std::size_t>(d6) >= count) {
                continue;
            }

            const std::int16_t y = group2[static_cast<std::size_t>(d6)].y;
            if (y > max_y) {
                max_y = y;
                best_slot = d6;
            }
        }

        if (best_slot < 0) {
            break;
        }

        done_mask |= (1U << static_cast<std::uint32_t>(best_slot));
        order.push_back(static_cast<std::size_t>(best_slot));
    }

    if (out_done_mask != nullptr) {
        *out_done_mask = done_mask;
    }
    return order;
}

ShadowSatPassResult dispatch_shadow_sat_pass(
    std::span<const ShadowEntitySlot> group1,
    std::span<const ShadowEntitySlot> group2,
    std::span<const ShadowEntitySlot> group4,
    std::span<const ShadowEntitySlot> group3,
    std::uint16_t current_sat_offset) {
    ShadowSatPassResult result{};

    // 1. Pass 1: Group 1 High-Priority FX ($FF2954, 4 slots)
    // ROM 0x00A6A4..0x00A6C2 checks type > 0 and bit 2 of +0x3A
    const std::size_t g1_count = std::min(group1.size(), ShadowSatPassConfig::kGroup1SlotCount);
    for (std::size_t i = 0U; i < g1_count; ++i) {
        const auto& ent = group1[i];
        if (ent.has_shadow()) {
            ShadowDispatchEntry entry{};
            entry.pool = SatPoolKind::Group1HighPriority;
            entry.slot_index = i;
            entry.type = ent.type;
            entry.y = ent.y;
            entry.ground_y = ent.ground_contact_y();
            entry.tier = ent.shadow_tier();
            entry.target = (ent.flags_36 & 0x01U) ? SatDispatchTarget::CustomSat : SatDispatchTarget::NormalSat;
            entry.ram_addr = ShadowSatPassConfig::kGroup1PoolRam +
                             static_cast<std::uint32_t>(i * ShadowSatPassConfig::kGroup1SlotStride);
            result.entries.push_back(entry);
        }
    }

    // 2. Pass 2: Group 2 Shadows ($FF19E8, 21 slots, Y-depth sorted)
    const std::vector<std::size_t> g2_sorted = sort_group2_shadows_y_depth_order(group2, &result.shadow_done_mask);
    for (std::size_t idx : g2_sorted) {
        const auto& ent = group2[idx];
        ShadowDispatchEntry entry{};
        entry.pool = SatPoolKind::Group2Entities;
        entry.slot_index = idx;
        entry.type = ent.type;
        entry.y = ent.y;
        entry.ground_y = ent.ground_contact_y();
        entry.tier = ent.shadow_tier();
        entry.target = (ent.flags_36 & 0x01U) ? SatDispatchTarget::CustomSat : SatDispatchTarget::NormalSat;
        entry.ram_addr = ShadowSatPassConfig::kGroup2PoolRam +
                         static_cast<std::uint32_t>(idx * ShadowSatPassConfig::kGroup2SlotStride);
        result.entries.push_back(entry);
    }

    // 3. Pass 3: Group 4 Ground/Shadow FX ($FF2ABC, 8 slots)
    const std::size_t g4_count = std::min(group4.size(), ShadowSatPassConfig::kGroup4SlotCount);
    for (std::size_t i = 0U; i < g4_count; ++i) {
        const auto& ent = group4[i];
        if (ent.type > 0) {
            ShadowDispatchEntry entry{};
            entry.pool = SatPoolKind::Group4GroundShadow;
            entry.slot_index = i;
            entry.type = ent.type;
            entry.y = ent.y;
            entry.ground_y = ent.ground_contact_y();
            entry.tier = ent.shadow_tier();
            entry.target = (ent.flags_36 & 0x01U) ? SatDispatchTarget::CustomSat : SatDispatchTarget::NormalSat;
            entry.ram_addr = ShadowSatPassConfig::kGroup4PoolRam +
                             static_cast<std::uint32_t>(i * ShadowSatPassConfig::kGroup4SlotStride);
            result.entries.push_back(entry);
        }
    }

    // 4. Pass 4: Group 3 Floor FX ($FF2D8C, 6 slots)
    const std::size_t g3_count = std::min(group3.size(), ShadowSatPassConfig::kGroup3SlotCount);
    for (std::size_t i = 0U; i < g3_count; ++i) {
        const auto& ent = group3[i];
        if (ent.type > 0) {
            ShadowDispatchEntry entry{};
            entry.pool = SatPoolKind::Group3Floor;
            entry.slot_index = i;
            entry.type = ent.type;
            entry.y = ent.y;
            entry.ground_y = ent.ground_contact_y();
            entry.tier = ent.shadow_tier();
            entry.target = (ent.flags_36 & 0x01U) ? SatDispatchTarget::CustomSat : SatDispatchTarget::NormalSat;
            entry.ram_addr = ShadowSatPassConfig::kGroup3PoolRam +
                             static_cast<std::uint32_t>(i * ShadowSatPassConfig::kGroup3SlotStride);
            result.entries.push_back(entry);
        }
    }

    // 5. SAT Link Terminator (0x00A76A..0x00A784)
    result.sat_terminated = true;
    if (current_sat_offset == 0U) {
        result.terminated_sat_offset = 8U;
    } else {
        result.terminated_sat_offset = current_sat_offset;
    }

    return result;
}

ShadowSatPassResult dispatch_shadow_sat_raw(
    std::span<std::uint8_t> ram_buffer,
    std::uint32_t ram_base_addr) {
    auto ram_offset = [ram_base_addr, buf_size = ram_buffer.size()](std::uint32_t addr) -> std::size_t {
        if (addr < ram_base_addr || (addr - ram_base_addr) >= buf_size) {
            return buf_size;
        }
        return static_cast<std::size_t>(addr - ram_base_addr);
    };

    const std::size_t sat_off_pos = ram_offset(ShadowSatPassConfig::kSatByteOffsetRam);
    const std::uint16_t cur_sat_offset = read_u16_be(ram_buffer, sat_off_pos);

    auto read_shadow_slots = [&](std::uint32_t base_addr, std::size_t count, std::size_t stride) {
        std::vector<ShadowEntitySlot> slots(count);
        for (std::size_t i = 0U; i < count; ++i) {
            const std::size_t off = ram_offset(base_addr + static_cast<std::uint32_t>(i * stride));
            if (off < ram_buffer.size()) {
                slots[i].type = read_i16_be(ram_buffer, off + ShadowSatPassConfig::kTypeOffset);
                slots[i].x = read_i16_be(ram_buffer, off + ShadowSatPassConfig::kXOffset);
                slots[i].y = read_i16_be(ram_buffer, off + ShadowSatPassConfig::kYOffset);
                slots[i].z = read_i16_be(ram_buffer, off + ShadowSatPassConfig::kZOffset);
                slots[i].ground_h = read_i16_be(ram_buffer, off + ShadowSatPassConfig::kGroundHOffset);
                const std::size_t f36_off = off + ShadowSatPassConfig::kFlags36Offset;
                if (f36_off < ram_buffer.size()) {
                    slots[i].flags_36 = ram_buffer[f36_off];
                }
                const std::size_t f3a_off = off + ShadowSatPassConfig::kFlags3AOffset;
                if (f3a_off < ram_buffer.size()) {
                    slots[i].flags_3a = ram_buffer[f3a_off];
                }
            }
        }
        return slots;
    };

    const auto g1 = read_shadow_slots(ShadowSatPassConfig::kGroup1PoolRam,
                                      ShadowSatPassConfig::kGroup1SlotCount,
                                      ShadowSatPassConfig::kGroup1SlotStride);
    const auto g2 = read_shadow_slots(ShadowSatPassConfig::kGroup2PoolRam,
                                      ShadowSatPassConfig::kGroup2SlotCount,
                                      ShadowSatPassConfig::kGroup2SlotStride);
    const auto g4 = read_shadow_slots(ShadowSatPassConfig::kGroup4PoolRam,
                                      ShadowSatPassConfig::kGroup4SlotCount,
                                      ShadowSatPassConfig::kGroup4SlotStride);
    const auto g3 = read_shadow_slots(ShadowSatPassConfig::kGroup3PoolRam,
                                      ShadowSatPassConfig::kGroup3SlotCount,
                                      ShadowSatPassConfig::kGroup3SlotStride);

    ShadowSatPassResult res = dispatch_shadow_sat_pass(g1, g2, g4, g3, cur_sat_offset);

    // Terminate SAT link byte at -5(a6, d7.w)
    const std::uint32_t link_addr = ShadowSatPassConfig::kVdpSatBufferRam +
                                    res.terminated_sat_offset - 5U;
    const std::size_t link_pos = ram_offset(link_addr);
    if (link_pos < ram_buffer.size()) {
        ram_buffer[link_pos] = 0U;
    }

    return res;
}

} // namespace oasis::game::render
