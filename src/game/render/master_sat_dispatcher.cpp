#include "game/render/master_sat_dispatcher.hpp"

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

inline void write_u32_be(std::span<std::uint8_t> buf, std::size_t offset, std::uint32_t val) noexcept {
    if (offset + 3 < buf.size()) {
        buf[offset] = static_cast<std::uint8_t>((val >> 24) & 0xFFU);
        buf[offset + 1] = static_cast<std::uint8_t>((val >> 16) & 0xFFU);
        buf[offset + 2] = static_cast<std::uint8_t>((val >> 8) & 0xFFU);
        buf[offset + 3] = static_cast<std::uint8_t>(val & 0xFFU);
    }
}

} // namespace

std::uint32_t build_group2_inactive_mask(
    std::span<const SatEntitySlot> group2) noexcept {
    std::uint32_t mask = 0U;
    const std::size_t count = std::min(group2.size(), MasterSatDispatcherConfig::kGroup2SlotCount);
    for (std::size_t i = 0U; i < count; ++i) {
        if (group2[i].type == 0) {
            mask |= (1U << i);
        }
    }
    // Any remaining slots up to 21 are considered inactive
    for (std::size_t i = count; i < MasterSatDispatcherConfig::kGroup2SlotCount; ++i) {
        mask |= (1U << i);
    }
    return mask;
}

std::vector<std::size_t> sort_group2_y_depth_order(
    std::span<const SatEntitySlot> group2,
    std::uint32_t* out_done_mask) {
    std::vector<std::size_t> order;
    std::uint32_t done_mask = build_group2_inactive_mask(group2);
    const std::size_t count = std::min(group2.size(), MasterSatDispatcherConfig::kGroup2SlotCount);

    while (true) {
        std::int16_t max_y = MasterSatDispatcherConfig::kInitialMaxYSentinel;
        int best_slot = -1;

        // ROM 0x00A1F6..0x00A220 scans from d6 = 20 down to 0
        for (int d6 = static_cast<int>(MasterSatDispatcherConfig::kGroup2SlotCount) - 1; d6 >= 0; --d6) {
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

MasterSatDispatchResult dispatch_master_sat_pass(
    bool demo_active,
    bool foreground_player_override,
    std::span<const SatEntitySlot> group1,
    std::span<const SatEntitySlot> special,
    std::span<const SatEntitySlot> group2,
    std::span<const SatEntitySlot> group4,
    std::span<const SatEntitySlot> group3,
    std::uint16_t current_sat_offset,
    std::uint8_t initial_vdp_sync_flags) {
    MasterSatDispatchResult result{};
    result.composite_flag_cleared = true;
    result.vdp_sync_flags = initial_vdp_sync_flags;

    // 0x00A1A0..0x00A1A6: demo mode active branch
    if (demo_active) {
        result.demo_branch_taken = true;
        return result;
    }

    // 1. Pass 1: Group 1 High-Priority FX ($FF2954, 4 slots)
    const std::size_t g1_count = std::min(group1.size(), MasterSatDispatcherConfig::kGroup1SlotCount);
    for (std::size_t i = 0U; i < g1_count; ++i) {
        const auto& ent = group1[i];
        if (ent.type != 0) {
            SatDispatchEntry entry{};
            entry.pool = SatPoolKind::Group1HighPriority;
            entry.slot_index = i;
            entry.type = ent.type;
            entry.y = ent.y;
            entry.flags_36 = ent.flags_36;
            entry.target = (ent.flags_36 & 0x01U) ? SatDispatchTarget::CustomSat : SatDispatchTarget::NormalSat;
            entry.ram_addr = MasterSatDispatcherConfig::kGroup1PoolRam +
                             static_cast<std::uint32_t>(i * MasterSatDispatcherConfig::kGroup1SlotStride);
            result.entries.push_back(entry);
        }
    }

    // 2. Pass 2: Special Pool / Hit-Sparks ($FF19C8, 4 slots)
    const std::size_t sp_count = std::min(special.size(), MasterSatDispatcherConfig::kSpecialSlotCount);
    for (std::size_t i = 0U; i < sp_count; ++i) {
        const auto& ent = special[i];
        if (ent.type != 0) {
            SatDispatchEntry entry{};
            entry.pool = SatPoolKind::SpecialHitSparks;
            entry.slot_index = i;
            entry.type = ent.type;
            entry.y = ent.y;
            entry.flags_36 = ent.flags_36;
            entry.target = SatDispatchTarget::SpecialJumpTable;
            entry.ram_addr = MasterSatDispatcherConfig::kSpecialPoolRam +
                             static_cast<std::uint32_t>(i * MasterSatDispatcherConfig::kSpecialSlotStride);
            result.entries.push_back(entry);
        }
    }

    // 3. Pass 3: Group 2 Entities ($FF19E8, 21 slots, Y-depth sorted)
    const std::vector<std::size_t> g2_sorted = sort_group2_y_depth_order(group2, &result.entity_done_mask);
    for (std::size_t idx : g2_sorted) {
        const auto& ent = group2[idx];
        SatDispatchEntry entry{};
        entry.pool = SatPoolKind::Group2Entities;
        entry.slot_index = idx;
        entry.type = ent.type;
        entry.y = ent.y;
        entry.flags_36 = ent.flags_36;
        entry.target = (ent.flags_36 & 0x01U) ? SatDispatchTarget::CustomSat : SatDispatchTarget::NormalSat;
        entry.ram_addr = MasterSatDispatcherConfig::kGroup2PoolRam +
                         static_cast<std::uint32_t>(idx * MasterSatDispatcherConfig::kGroup2SlotStride);
        result.entries.push_back(entry);
    }

    // 4. Pass 4: Group 4 Ground/Shadow FX ($FF2ABC, 8 slots)
    const std::size_t g4_count = std::min(group4.size(), MasterSatDispatcherConfig::kGroup4SlotCount);
    for (std::size_t i = 0U; i < g4_count; ++i) {
        const auto& ent = group4[i];
        if (ent.type != 0) {
            SatDispatchEntry entry{};
            entry.pool = SatPoolKind::Group4GroundShadow;
            entry.slot_index = i;
            entry.type = ent.type;
            entry.y = ent.y;
            entry.flags_36 = ent.flags_36;
            entry.target = (ent.flags_36 & 0x01U) ? SatDispatchTarget::CustomSat : SatDispatchTarget::NormalSat;
            entry.ram_addr = MasterSatDispatcherConfig::kGroup4PoolRam +
                             static_cast<std::uint32_t>(i * MasterSatDispatcherConfig::kGroup4SlotStride);
            result.entries.push_back(entry);
        }
    }

    // 5. Pass 5: Group 3 Floor FX ($FF2D8C, 6 slots)
    const std::size_t g3_count = std::min(group3.size(), MasterSatDispatcherConfig::kGroup3SlotCount);
    for (std::size_t i = 0U; i < g3_count; ++i) {
        const auto& ent = group3[i];
        if (ent.type != 0) {
            SatDispatchEntry entry{};
            entry.pool = SatPoolKind::Group3Floor;
            entry.slot_index = i;
            entry.type = ent.type;
            entry.y = ent.y;
            entry.flags_36 = ent.flags_36;
            entry.target = (ent.flags_36 & 0x01U) ? SatDispatchTarget::CustomSat : SatDispatchTarget::NormalSat;
            entry.ram_addr = MasterSatDispatcherConfig::kGroup3PoolRam +
                             static_cast<std::uint32_t>(i * MasterSatDispatcherConfig::kGroup3SlotStride);
            result.entries.push_back(entry);
        }
    }

    // 6. Foreground Player Override ($FF1995 non-zero and slot 0 active)
    if (foreground_player_override && !group2.empty() && group2[0].type != 0) {
        const auto& player = group2[0];
        SatDispatchEntry entry{};
        entry.pool = SatPoolKind::ForegroundPlayer;
        entry.slot_index = 0U;
        entry.type = player.type;
        entry.y = player.y;
        entry.flags_36 = player.flags_36;
        entry.target = (player.flags_36 & 0x01U) ? SatDispatchTarget::CustomSatForeground
                                                : SatDispatchTarget::NormalSatForeground;
        entry.ram_addr = MasterSatDispatcherConfig::kGroup2PoolRam;
        result.entries.push_back(entry);
    }

    // 7. SAT Link Terminator (0x00A2A6..0x00A2C6)
    result.sat_terminated = true;
    if (current_sat_offset == 0U) {
        result.terminated_sat_offset = 8U;
    } else {
        result.terminated_sat_offset = current_sat_offset;
    }
    result.vdp_sync_flags |= MasterSatDispatcherConfig::kVdpSyncMask;

    return result;
}

MasterSatDispatchResult dispatch_master_sat_raw(
    std::span<std::uint8_t> ram_buffer,
    std::uint32_t ram_base_addr) {
    auto ram_offset = [ram_base_addr, buf_size = ram_buffer.size()](std::uint32_t addr) -> std::size_t {
        if (addr < ram_base_addr || (addr - ram_base_addr) >= buf_size) {
            return buf_size; // out of range
        }
        return static_cast<std::size_t>(addr - ram_base_addr);
    };

    // sf.b $FF1651
    const std::size_t comp_off = ram_offset(MasterSatDispatcherConfig::kCompositeFlagRam);
    if (comp_off < ram_buffer.size()) {
        ram_buffer[comp_off] = 0U;
    }

    // Read demo flag $FF1996
    const std::size_t demo_off = ram_offset(MasterSatDispatcherConfig::kDemoActiveFlagRam);
    const bool demo_active = (demo_off < ram_buffer.size()) && (ram_buffer[demo_off] != 0U);

    // Read foreground override flag $FF1995
    const std::size_t fg_off = ram_offset(MasterSatDispatcherConfig::kForegroundPlayerFlagRam);
    const bool fg_override = (fg_off < ram_buffer.size()) && (ram_buffer[fg_off] != 0U);

    // Read current sat offset $FF188C
    const std::size_t sat_off_pos = ram_offset(MasterSatDispatcherConfig::kSatByteOffsetRam);
    const std::uint16_t cur_sat_offset = read_u16_be(ram_buffer, sat_off_pos);

    // Read initial VDP sync flags $FF164E
    const std::size_t sync_pos = ram_offset(MasterSatDispatcherConfig::kVdpSyncFlagsRam);
    const std::uint8_t init_sync = (sync_pos < ram_buffer.size()) ? ram_buffer[sync_pos] : 0U;

    // Helper to read pool of entities
    auto read_slots = [&](std::uint32_t base_addr, std::size_t count, std::size_t stride) {
        std::vector<SatEntitySlot> slots(count);
        for (std::size_t i = 0U; i < count; ++i) {
            const std::size_t off = ram_offset(base_addr + static_cast<std::uint32_t>(i * stride));
            if (off < ram_buffer.size()) {
                slots[i].type = read_i16_be(ram_buffer, off + MasterSatDispatcherConfig::kTypeOffset);
                slots[i].y = read_i16_be(ram_buffer, off + MasterSatDispatcherConfig::kYOffset);
                const std::size_t f36_off = off + MasterSatDispatcherConfig::kFlags36Offset;
                if (f36_off < ram_buffer.size()) {
                    slots[i].flags_36 = ram_buffer[f36_off];
                }
            }
        }
        return slots;
    };

    const auto g1 = read_slots(MasterSatDispatcherConfig::kGroup1PoolRam,
                               MasterSatDispatcherConfig::kGroup1SlotCount,
                               MasterSatDispatcherConfig::kGroup1SlotStride);
    const auto sp = read_slots(MasterSatDispatcherConfig::kSpecialPoolRam,
                               MasterSatDispatcherConfig::kSpecialSlotCount,
                               MasterSatDispatcherConfig::kSpecialSlotStride);
    const auto g2 = read_slots(MasterSatDispatcherConfig::kGroup2PoolRam,
                               MasterSatDispatcherConfig::kGroup2SlotCount,
                               MasterSatDispatcherConfig::kGroup2SlotStride);
    const auto g4 = read_slots(MasterSatDispatcherConfig::kGroup4PoolRam,
                               MasterSatDispatcherConfig::kGroup4SlotCount,
                               MasterSatDispatcherConfig::kGroup4SlotStride);
    const auto g3 = read_slots(MasterSatDispatcherConfig::kGroup3PoolRam,
                               MasterSatDispatcherConfig::kGroup3SlotCount,
                               MasterSatDispatcherConfig::kGroup3SlotStride);

    MasterSatDispatchResult res = dispatch_master_sat_pass(
        demo_active, fg_override, g1, sp, g2, g4, g3, cur_sat_offset, init_sync);

    // Write back $FF188E (entity done mask)
    const std::size_t mask_off = ram_offset(MasterSatDispatcherConfig::kEntityDoneMaskRam);
    write_u32_be(ram_buffer, mask_off, res.entity_done_mask);

    if (!demo_active) {
        // Write link terminator at -5(a6, d7.w) where a6 = kVdpSatBufferRam
        const std::uint32_t link_addr = MasterSatDispatcherConfig::kVdpSatBufferRam +
                                        res.terminated_sat_offset - 5U;
        const std::size_t link_pos = ram_offset(link_addr);
        if (link_pos < ram_buffer.size()) {
            ram_buffer[link_pos] = 0U; // link = 0
        }

        // Write back updated sync flags to $FF164E
        if (sync_pos < ram_buffer.size()) {
            ram_buffer[sync_pos] = res.vdp_sync_flags;
        }
    }

    return res;
}

} // namespace oasis::game::render
