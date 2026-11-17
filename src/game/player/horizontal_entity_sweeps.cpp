#include "game/player/horizontal_entity_sweeps.hpp"

#include <vector>

namespace oasis::collision {

namespace {

inline std::int16_t read_i16_be(std::span<const std::uint8_t> bytes, std::size_t offset) noexcept {
    if (offset + 1U >= bytes.size()) {
        return 0;
    }
    return static_cast<std::int16_t>(
        (static_cast<std::uint16_t>(bytes[offset]) << 8U) |
         static_cast<std::uint16_t>(bytes[offset + 1U]));
}

inline void write_i16_be(std::span<std::uint8_t> bytes, std::size_t offset, std::int16_t val) noexcept {
    if (offset + 1U < bytes.size()) {
        bytes[offset] = static_cast<std::uint8_t>((static_cast<std::uint16_t>(val) >> 8U) & 0xFFU);
        bytes[offset + 1U] = static_cast<std::uint8_t>(static_cast<std::uint16_t>(val) & 0xFFU);
    }
}

inline void write_u32_be(std::span<std::uint8_t> bytes, std::size_t offset, std::uint32_t val) noexcept {
    if (offset + 3U < bytes.size()) {
        bytes[offset] = static_cast<std::uint8_t>((val >> 24U) & 0xFFU);
        bytes[offset + 1U] = static_cast<std::uint8_t>((val >> 16U) & 0xFFU);
        bytes[offset + 2U] = static_cast<std::uint8_t>((val >> 8U) & 0xFFU);
        bytes[offset + 3U] = static_cast<std::uint8_t>(val & 0xFFU);
    }
}

inline DynamicEntityHeader load_header_from_slot(
    std::span<const std::uint8_t> slot,
    std::uint32_t slot_id) noexcept {
    DynamicEntityHeader h{};
    h.slot_id = slot_id;
    if (slot.size() < 0x74U) {
        return h;
    }
    h.type = read_i16_be(slot, HorizontalEntitySweepsConfig::kTypeOffset);
    h.flags_37 = slot[HorizontalEntitySweepsConfig::kFlags37Offset];
    h.flags_38 = slot[HorizontalEntitySweepsConfig::kFlags38Offset];
    h.flags_3a = slot[HorizontalEntitySweepsConfig::kFlags3AOffset];
    h.collision_flags_x = slot[HorizontalEntitySweepsConfig::kCollisionFlagsXOffset];
    h.reaction_code = slot[HorizontalEntitySweepsConfig::kReactionCodeOffset];
    h.x = read_i16_be(slot, HorizontalEntitySweepsConfig::kXOffset);
    h.y = read_i16_be(slot, HorizontalEntitySweepsConfig::kYOffset);
    h.z = read_i16_be(slot, HorizontalEntitySweepsConfig::kZOffset);
    h.ground_h = read_i16_be(slot, HorizontalEntitySweepsConfig::kGroundHOffset);
    h.rad_x = read_i16_be(slot, HorizontalEntitySweepsConfig::kRadXOffset);
    h.rad_y = read_i16_be(slot, HorizontalEntitySweepsConfig::kRadYOffset);
    h.height = read_i16_be(slot, HorizontalEntitySweepsConfig::kHeightOffset);
    h.vel_x = read_i16_be(slot, HorizontalEntitySweepsConfig::kVelXOffset);
    return h;
}

} // namespace

std::uint16_t sweep_entities_pos_x(
    std::uint16_t dx,
    DynamicEntityHeader& self,
    std::span<DynamicEntityHeader> pool) noexcept {
    // 0x009DD6: btst.b #2, $38(a6)
    if ((self.flags_38 & HorizontalEntitySweepsConfig::kFlagIntangibleMask) != 0U) {
        return dx;
    }

    const auto self_right = static_cast<std::int16_t>(self.x + self.rad_x);
    const auto self_elev = static_cast<std::int16_t>(self.z + self.ground_h);
    const auto self_y_base = static_cast<std::int16_t>(self.y + self.ground_h);
    const auto self_top_y = static_cast<std::int16_t>(self_y_base - self.rad_y);
    const auto self_bot_y = static_cast<std::int16_t>(self_y_base + self.rad_y);
    const auto self_top_z = static_cast<std::int16_t>(self_elev + self.height);

    for (auto& target : pool) {
        // 0x009E06: tst.w $0(a5); ble
        if (target.type <= 0) {
            continue;
        }
        // 0x009E0E: btst.b #2, $38(a5); bne
        if ((target.flags_38 & HorizontalEntitySweepsConfig::kFlagIntangibleMask) != 0U) {
            continue;
        }
        // 0x009E18: btst.b #1, $3a(a5); bne
        if ((target.flags_3a & HorizontalEntitySweepsConfig::kFlagIgnoreCollisionMask) != 0U) {
            continue;
        }
        // 0x009E22: move.w $8(a5), d6; cmp.w $8(a6), d6; ble
        if (target.x <= self.x) {
            continue;
        }

        // 0x009E2E: sub.w $42(a5), d6 (target left edge)
        // 0x009E32: sub.w d1, d6 (distance = target left - self right)
        auto dist = static_cast<std::int16_t>((target.x - target.rad_x) - self_right);
        if (dist < 0) {
            dist = 0;
        }
        // 0x009E3A: cmp.w d0, d6; bge
        if (static_cast<std::uint16_t>(dist) >= dx) {
            continue;
        }

        // Y overlap:
        const auto tgt_y_base = static_cast<std::int16_t>(target.y + target.ground_h);
        const auto tgt_top_y = static_cast<std::int16_t>(tgt_y_base - target.rad_y);
        const auto tgt_bot_y = static_cast<std::int16_t>(tgt_y_base + target.rad_y);
        // 0x009E52: cmp.w d2, d5; ble
        if (tgt_bot_y <= self_top_y) {
            continue;
        }
        // 0x009E58: cmp.w d3, d4; bge
        if (tgt_top_y >= self_bot_y) {
            continue;
        }

        // Z overlap:
        const auto tgt_elev = static_cast<std::int16_t>(target.z + target.ground_h);
        const auto tgt_top_z = static_cast<std::int16_t>(tgt_elev + target.height);
        // 0x009E6C: cmp.w a2, d5; ble
        if (tgt_top_z <= self_elev) {
            continue;
        }
        // 0x009E78: cmp.w a1, d4; bge
        if (tgt_elev >= self_top_z) {
            continue;
        }

        // 0x009E7E: bset.b #7, $40(a6)
        self.collision_flags_x |= HorizontalEntitySweepsConfig::kCollisionFlagEntityHit;
        // 0x009E84: move.l a5, $68(a6)
        self.collided_entity_id = target.slot_id;
        // 0x009E88: move.b #3, $6c(a6)
        self.reaction_code = HorizontalEntitySweepsConfig::kReactionCodeSolidEntity;
        // 0x009E8E: move.w d6, d0
        dx = static_cast<std::uint16_t>(dist);
        if (dx == 0U) {
            // 0x009E94: btst.b #5, $37(a6); bne
            // 0x009E9E: btst.b #6, $37(a5); bne
            if ((self.flags_37 & HorizontalEntitySweepsConfig::kFlagCannotPushMask) == 0U &&
                (target.flags_37 & HorizontalEntitySweepsConfig::kFlagCannotBePushedMask) == 0U) {
                // 0x009EA8: addq.w #1, $72(a5)
                target.vel_x = static_cast<std::int16_t>(target.vel_x + 1);
            }
        }
    }
    return dx;
}

std::uint16_t sweep_entities_neg_x(
    std::uint16_t dx,
    DynamicEntityHeader& self,
    std::span<DynamicEntityHeader> pool) noexcept {
    // 0x009EBA: btst.b #2, $38(a6)
    if ((self.flags_38 & HorizontalEntitySweepsConfig::kFlagIntangibleMask) != 0U) {
        return dx;
    }

    const auto self_left = static_cast<std::int16_t>(self.x - self.rad_x);
    const auto self_elev = static_cast<std::int16_t>(self.z + self.ground_h);
    const auto self_y_base = static_cast<std::int16_t>(self.y + self.ground_h);
    const auto self_top_y = static_cast<std::int16_t>(self_y_base - self.rad_y);
    const auto self_bot_y = static_cast<std::int16_t>(self_y_base + self.rad_y);
    const auto self_top_z = static_cast<std::int16_t>(self_elev + self.height);

    for (auto& target : pool) {
        // 0x009EEA: tst.w $0(a5); ble
        if (target.type <= 0) {
            continue;
        }
        // 0x009EF2: btst.b #2, $38(a5); bne
        if ((target.flags_38 & HorizontalEntitySweepsConfig::kFlagIntangibleMask) != 0U) {
            continue;
        }
        // 0x009EFC: btst.b #1, $3a(a5); bne
        if ((target.flags_3a & HorizontalEntitySweepsConfig::kFlagIgnoreCollisionMask) != 0U) {
            continue;
        }
        // 0x009F06: move.w $8(a5), d6; cmp.w $8(a6), d6; bge
        if (target.x >= self.x) {
            continue;
        }

        // 0x009F12: add.w $42(a5), d6 (target right edge)
        // 0x009F16: sub.w d1, d6 (target right edge - self left edge)
        // 0x009F18: neg.w d6 (self left edge - target right edge)
        auto dist = static_cast<std::int16_t>(self_left - (target.x + target.rad_x));
        if (dist < 0) {
            dist = 0;
        }
        // 0x009F20: cmp.w d0, d6; bge
        if (static_cast<std::uint16_t>(dist) >= dx) {
            continue;
        }

        // Y overlap:
        const auto tgt_y_base = static_cast<std::int16_t>(target.y + target.ground_h);
        const auto tgt_top_y = static_cast<std::int16_t>(tgt_y_base - target.rad_y);
        const auto tgt_bot_y = static_cast<std::int16_t>(tgt_y_base + target.rad_y);
        // 0x009F38: cmp.w d2, d5; ble
        if (tgt_bot_y <= self_top_y) {
            continue;
        }
        // 0x009F3E: cmp.w d3, d4; bge
        if (tgt_top_y >= self_bot_y) {
            continue;
        }

        // Z overlap:
        const auto tgt_elev = static_cast<std::int16_t>(target.z + target.ground_h);
        const auto tgt_top_z = static_cast<std::int16_t>(tgt_elev + target.height);
        // 0x009F52: cmp.w a2, d5; ble
        if (tgt_top_z <= self_elev) {
            continue;
        }
        // 0x009F5E: cmp.w a1, d4; bge
        if (tgt_elev >= self_top_z) {
            continue;
        }

        // 0x009F64: bset.b #7, $40(a6)
        self.collision_flags_x |= HorizontalEntitySweepsConfig::kCollisionFlagEntityHit;
        // 0x009F6A: move.l a5, $68(a6)
        self.collided_entity_id = target.slot_id;
        // 0x009F6E: move.b #3, $6c(a6)
        self.reaction_code = HorizontalEntitySweepsConfig::kReactionCodeSolidEntity;
        // 0x009F74: move.w d6, d0
        dx = static_cast<std::uint16_t>(dist);
        if (dx == 0U) {
            // 0x009F7A: btst.b #5, $37(a6); bne
            // 0x009F84: btst.b #6, $37(a5); bne
            if ((self.flags_37 & HorizontalEntitySweepsConfig::kFlagCannotPushMask) == 0U &&
                (target.flags_37 & HorizontalEntitySweepsConfig::kFlagCannotBePushedMask) == 0U) {
                // 0x009F8E: subq.w #1, $72(a5)
                target.vel_x = static_cast<std::int16_t>(target.vel_x - 1);
            }
        }
    }
    return dx;
}

std::uint16_t sweep_slot_entities_pos_x(
    std::uint16_t dx,
    std::span<std::uint8_t> self_slot,
    std::span<std::uint8_t> pool_bytes,
    std::size_t slot_count,
    std::size_t slot_stride,
    std::uint32_t pool_base_addr) noexcept {
    if (self_slot.size() < 0x74U) {
        return dx;
    }

    auto self = load_header_from_slot(self_slot, 0U);
    std::vector<DynamicEntityHeader> pool;
    pool.reserve(slot_count);

    for (std::size_t i = 0; i < slot_count; ++i) {
        const auto slot_off = i * slot_stride;
        if (slot_off + slot_stride > pool_bytes.size()) {
            break;
        }
        auto slot_span = pool_bytes.subspan(slot_off, slot_stride);
        const auto slot_addr = static_cast<std::uint32_t>(pool_base_addr + slot_off);
        pool.push_back(load_header_from_slot(slot_span, slot_addr));
    }

    dx = sweep_entities_pos_x(dx, self, pool);

    self_slot[HorizontalEntitySweepsConfig::kCollisionFlagsXOffset] = self.collision_flags_x;
    self_slot[HorizontalEntitySweepsConfig::kReactionCodeOffset] = self.reaction_code;
    write_u32_be(self_slot, HorizontalEntitySweepsConfig::kCollidedPtrOffset, self.collided_entity_id);

    // Write back any velocity updates to target slots
    for (std::size_t i = 0; i < pool.size(); ++i) {
        const auto slot_off = i * slot_stride;
        auto slot_span = pool_bytes.subspan(slot_off, slot_stride);
        write_i16_be(slot_span, HorizontalEntitySweepsConfig::kVelXOffset, pool[i].vel_x);
    }

    return dx;
}

std::uint16_t sweep_slot_entities_neg_x(
    std::uint16_t dx,
    std::span<std::uint8_t> self_slot,
    std::span<std::uint8_t> pool_bytes,
    std::size_t slot_count,
    std::size_t slot_stride,
    std::uint32_t pool_base_addr) noexcept {
    if (self_slot.size() < 0x74U) {
        return dx;
    }

    auto self = load_header_from_slot(self_slot, 0U);
    std::vector<DynamicEntityHeader> pool;
    pool.reserve(slot_count);

    for (std::size_t i = 0; i < slot_count; ++i) {
        const auto slot_off = i * slot_stride;
        if (slot_off + slot_stride > pool_bytes.size()) {
            break;
        }
        auto slot_span = pool_bytes.subspan(slot_off, slot_stride);
        const auto slot_addr = static_cast<std::uint32_t>(pool_base_addr + slot_off);
        pool.push_back(load_header_from_slot(slot_span, slot_addr));
    }

    dx = sweep_entities_neg_x(dx, self, pool);

    self_slot[HorizontalEntitySweepsConfig::kCollisionFlagsXOffset] = self.collision_flags_x;
    self_slot[HorizontalEntitySweepsConfig::kReactionCodeOffset] = self.reaction_code;
    write_u32_be(self_slot, HorizontalEntitySweepsConfig::kCollidedPtrOffset, self.collided_entity_id);

    // Write back any velocity updates to target slots
    for (std::size_t i = 0; i < pool.size(); ++i) {
        const auto slot_off = i * slot_stride;
        auto slot_span = pool_bytes.subspan(slot_off, slot_stride);
        write_i16_be(slot_span, HorizontalEntitySweepsConfig::kVelXOffset, pool[i].vel_x);
    }

    return dx;
}

} // namespace oasis::collision
