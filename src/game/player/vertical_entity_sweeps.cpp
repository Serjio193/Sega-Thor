#include "game/player/vertical_entity_sweeps.hpp"

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

inline DynamicEntityHeaderY load_header_y_from_slot(
    std::span<const std::uint8_t> slot,
    std::uint32_t slot_id) noexcept {
    DynamicEntityHeaderY h{};
    h.slot_id = slot_id;
    if (slot.size() < 0x78U) {
        return h;
    }
    h.type = read_i16_be(slot, VerticalEntitySweepsConfig::kTypeOffset);
    h.flags_37 = slot[VerticalEntitySweepsConfig::kFlags37Offset];
    h.flags_38 = slot[VerticalEntitySweepsConfig::kFlags38Offset];
    h.flags_3a = slot[VerticalEntitySweepsConfig::kFlags3AOffset];
    h.collision_flags_y = slot[VerticalEntitySweepsConfig::kCollisionFlagsYOffset];
    h.reaction_code = slot[VerticalEntitySweepsConfig::kReactionCodeOffset];
    h.x = read_i16_be(slot, VerticalEntitySweepsConfig::kXOffset);
    h.y = read_i16_be(slot, VerticalEntitySweepsConfig::kYOffset);
    h.z = read_i16_be(slot, VerticalEntitySweepsConfig::kZOffset);
    h.ground_h = read_i16_be(slot, VerticalEntitySweepsConfig::kGroundHOffset);
    h.rad_x = read_i16_be(slot, VerticalEntitySweepsConfig::kRadXOffset);
    h.rad_y = read_i16_be(slot, VerticalEntitySweepsConfig::kRadYOffset);
    h.height = read_i16_be(slot, VerticalEntitySweepsConfig::kHeightOffset);
    h.vel_y = read_i16_be(slot, VerticalEntitySweepsConfig::kVelYOffset);
    return h;
}

} // namespace

std::uint16_t sweep_entities_pos_y(
    std::uint16_t dy,
    DynamicEntityHeaderY& self,
    std::span<DynamicEntityHeaderY> pool) noexcept {
    const auto self_bot_y = static_cast<std::int16_t>(self.y + self.ground_h + self.rad_y);
    const auto self_elev = static_cast<std::int16_t>(self.z + self.ground_h);
    const auto self_left_x = static_cast<std::int16_t>(self.x - self.rad_x);
    const auto self_right_x = static_cast<std::int16_t>(self.x + self.rad_x);
    const auto self_top_z = static_cast<std::int16_t>(self_elev + self.height);
    const bool self_is_ghost = (self.flags_38 & VerticalEntitySweepsConfig::kFlagIntangibleMask) != 0U;

    for (auto& target : pool) {
        // 0x009FD4: tst.w $0(a5); ble
        if (target.type <= 0) {
            continue;
        }
        // 0x009FDC: btst.b #1, $3a(a5); bne $9ff8
        if ((target.flags_3a & VerticalEntitySweepsConfig::kFlagIgnoreGhostMask) == 0U) {
            // 0x009FE6: btst.b #$1f, d7; bne
            if (self_is_ghost) {
                continue;
            }
            // 0x009FEE: btst.b #2, $38(a5); bne
            if ((target.flags_38 & VerticalEntitySweepsConfig::kFlagIntangibleMask) != 0U) {
                continue;
            }
        }

        // 0x009FF8: move.w $c(a6), d4; add.w $14(a6), d4
        // 0x00A000: move.w $c(a5), d6; add.w $14(a5), d6
        // 0x00A008: cmp.w d4, d6; ble
        const auto tgt_y_base = static_cast<std::int16_t>(target.y + target.ground_h);
        const auto self_y_base = static_cast<std::int16_t>(self.y + self.ground_h);
        if (tgt_y_base <= self_y_base) {
            continue;
        }

        // 0x00A00E: sub.w $44(a5), d6 (target top Y)
        // 0x00A012: sub.w d2, d6 (dist = target top Y - self bot Y)
        auto dist = static_cast<std::int16_t>((tgt_y_base - target.rad_y) - self_bot_y);
        if (dist < 0) {
            dist = 0;
        }
        // 0x00A01A: cmp.w d0, d6; bge
        if (static_cast<std::uint16_t>(dist) >= dy) {
            continue;
        }

        // X overlap:
        const auto tgt_left_x = static_cast<std::int16_t>(target.x - target.rad_x);
        const auto tgt_right_x = static_cast<std::int16_t>(target.x + target.rad_x);
        // 0x00A02E: cmp.w d1, d5; ble
        if (tgt_right_x <= self_left_x) {
            continue;
        }
        // 0x00A034: cmp.w d3, d4; bge
        if (tgt_left_x >= self_right_x) {
            continue;
        }

        // Z overlap:
        const auto tgt_elev = static_cast<std::int16_t>(target.z + target.ground_h);
        const auto tgt_top_z = static_cast<std::int16_t>(tgt_elev + target.height);
        // 0x00A048: cmp.w a2, d5; ble
        if (tgt_top_z <= self_elev) {
            continue;
        }
        // 0x00A054: cmp.w a1, d4; bge
        if (tgt_elev >= self_top_z) {
            continue;
        }

        // 0x00A05A: bset.b #7, $41(a6)
        self.collision_flags_y |= VerticalEntitySweepsConfig::kCollisionFlagEntityHit;
        // 0x00A060: move.l a5, $68(a6)
        self.collided_entity_id = target.slot_id;
        // 0x00A064: move.b #3, $6c(a6)
        self.reaction_code = VerticalEntitySweepsConfig::kReactionCodeSolidEntity;
        // 0x00A06A: move.w d6, d0
        dy = static_cast<std::uint16_t>(dist);
        if (dy == 0U) {
            // 0x00A070: btst.b #5, $37(a6); bne
            // 0x00A07A: btst.b #6, $37(a5); bne
            if ((self.flags_37 & VerticalEntitySweepsConfig::kFlagCannotPushMask) == 0U &&
                (target.flags_37 & VerticalEntitySweepsConfig::kFlagCannotBePushedMask) == 0U) {
                // 0x00A084: addq.w #1, $76(a5)
                target.vel_y = static_cast<std::int16_t>(target.vel_y + 1);
            }
        }
    }
    return dy;
}

std::uint16_t sweep_entities_neg_y(
    std::uint16_t dy,
    DynamicEntityHeaderY& self,
    std::span<DynamicEntityHeaderY> pool) noexcept {
    const auto self_top_y = static_cast<std::int16_t>(self.y + self.ground_h - self.rad_y);
    const auto self_elev = static_cast<std::int16_t>(self.z + self.ground_h);
    const auto self_left_x = static_cast<std::int16_t>(self.x - self.rad_x);
    const auto self_right_x = static_cast<std::int16_t>(self.x + self.rad_x);
    const auto self_top_z = static_cast<std::int16_t>(self_elev + self.height);
    const bool self_is_ghost = (self.flags_38 & VerticalEntitySweepsConfig::kFlagIntangibleMask) != 0U;

    for (auto& target : pool) {
        // 0x00A0CE: tst.w $0(a5); ble
        if (target.type <= 0) {
            continue;
        }
        // 0x00A0D6: btst.b #1, $3a(a5); bne
        if ((target.flags_3a & VerticalEntitySweepsConfig::kFlagIgnoreGhostMask) == 0U) {
            // 0x00A0E0: btst.b #$1f, d7; bne
            if (self_is_ghost) {
                continue;
            }
            // 0x00A0E8: btst.b #2, $38(a5); bne
            if ((target.flags_38 & VerticalEntitySweepsConfig::kFlagIntangibleMask) != 0U) {
                continue;
            }
        }

        const auto tgt_y_base = static_cast<std::int16_t>(target.y + target.ground_h);
        const auto self_y_base = static_cast<std::int16_t>(self.y + self.ground_h);
        // 0x00A102: cmp.w d4, d6; bge
        if (tgt_y_base >= self_y_base) {
            continue;
        }

        // 0x00A108: add.w $44(a5), d6 (target bot Y)
        // 0x00A10C: sub.w d2, d6
        // 0x00A10E: neg.w d6 (dist = self top Y - target bot Y)
        auto dist = static_cast<std::int16_t>(self_top_y - (tgt_y_base + target.rad_y));
        if (dist < 0) {
            dist = 0;
        }
        // 0x00A116: cmp.w d0, d6; bge
        if (static_cast<std::uint16_t>(dist) >= dy) {
            continue;
        }

        // X overlap:
        const auto tgt_left_x = static_cast<std::int16_t>(target.x - target.rad_x);
        const auto tgt_right_x = static_cast<std::int16_t>(target.x + target.rad_x);
        // 0x00A12A: cmp.w d1, d5; ble
        if (tgt_right_x <= self_left_x) {
            continue;
        }
        // 0x00A130: cmp.w d3, d4; bge
        if (tgt_left_x >= self_right_x) {
            continue;
        }

        // Z overlap:
        const auto tgt_elev = static_cast<std::int16_t>(target.z + target.ground_h);
        const auto tgt_top_z = static_cast<std::int16_t>(tgt_elev + target.height);
        // 0x00A144: cmp.w a2, d5; ble
        if (tgt_top_z <= self_elev) {
            continue;
        }
        // 0x00A150: cmp.w a1, d4; bge
        if (tgt_elev >= self_top_z) {
            continue;
        }

        // 0x00A156: bset.b #7, $41(a6)
        self.collision_flags_y |= VerticalEntitySweepsConfig::kCollisionFlagEntityHit;
        // 0x00A15C: move.l a5, $68(a6)
        self.collided_entity_id = target.slot_id;
        // 0x00A160: move.b #3, $6c(a6)
        self.reaction_code = VerticalEntitySweepsConfig::kReactionCodeSolidEntity;
        // 0x00A166: move.w d6, d0
        dy = static_cast<std::uint16_t>(dist);
        if (dy == 0U) {
            // 0x00A16C: btst.b #5, $37(a6); bne
            // 0x00A176: btst.b #6, $37(a5); bne
            if ((self.flags_37 & VerticalEntitySweepsConfig::kFlagCannotPushMask) == 0U &&
                (target.flags_37 & VerticalEntitySweepsConfig::kFlagCannotBePushedMask) == 0U) {
                // 0x00A180: subq.w #1, $76(a5)
                target.vel_y = static_cast<std::int16_t>(target.vel_y - 1);
            }
        }
    }
    return dy;
}

std::uint16_t sweep_slot_entities_pos_y(
    std::uint16_t dy,
    std::span<std::uint8_t> self_slot,
    std::span<std::uint8_t> pool_bytes,
    std::size_t slot_count,
    std::size_t slot_stride,
    std::uint32_t pool_base_addr) noexcept {
    if (self_slot.size() < 0x78U) {
        return dy;
    }

    auto self = load_header_y_from_slot(self_slot, 0U);
    std::vector<DynamicEntityHeaderY> pool;
    pool.reserve(slot_count);

    for (std::size_t i = 0; i < slot_count; ++i) {
        const auto slot_off = i * slot_stride;
        if (slot_off + slot_stride > pool_bytes.size()) {
            break;
        }
        auto slot_span = pool_bytes.subspan(slot_off, slot_stride);
        const auto slot_addr = static_cast<std::uint32_t>(pool_base_addr + slot_off);
        pool.push_back(load_header_y_from_slot(slot_span, slot_addr));
    }

    dy = sweep_entities_pos_y(dy, self, pool);

    self_slot[VerticalEntitySweepsConfig::kCollisionFlagsYOffset] = self.collision_flags_y;
    self_slot[VerticalEntitySweepsConfig::kReactionCodeOffset] = self.reaction_code;
    write_u32_be(self_slot, VerticalEntitySweepsConfig::kCollidedPtrOffset, self.collided_entity_id);

    // Write back any velocity updates to target slots
    for (std::size_t i = 0; i < pool.size(); ++i) {
        const auto slot_off = i * slot_stride;
        auto slot_span = pool_bytes.subspan(slot_off, slot_stride);
        write_i16_be(slot_span, VerticalEntitySweepsConfig::kVelYOffset, pool[i].vel_y);
    }

    return dy;
}

std::uint16_t sweep_slot_entities_neg_y(
    std::uint16_t dy,
    std::span<std::uint8_t> self_slot,
    std::span<std::uint8_t> pool_bytes,
    std::size_t slot_count,
    std::size_t slot_stride,
    std::uint32_t pool_base_addr) noexcept {
    if (self_slot.size() < 0x78U) {
        return dy;
    }

    auto self = load_header_y_from_slot(self_slot, 0U);
    std::vector<DynamicEntityHeaderY> pool;
    pool.reserve(slot_count);

    for (std::size_t i = 0; i < slot_count; ++i) {
        const auto slot_off = i * slot_stride;
        if (slot_off + slot_stride > pool_bytes.size()) {
            break;
        }
        auto slot_span = pool_bytes.subspan(slot_off, slot_stride);
        const auto slot_addr = static_cast<std::uint32_t>(pool_base_addr + slot_off);
        pool.push_back(load_header_y_from_slot(slot_span, slot_addr));
    }

    dy = sweep_entities_neg_y(dy, self, pool);

    self_slot[VerticalEntitySweepsConfig::kCollisionFlagsYOffset] = self.collision_flags_y;
    self_slot[VerticalEntitySweepsConfig::kReactionCodeOffset] = self.reaction_code;
    write_u32_be(self_slot, VerticalEntitySweepsConfig::kCollidedPtrOffset, self.collided_entity_id);

    // Write back any velocity updates to target slots
    for (std::size_t i = 0; i < pool.size(); ++i) {
        const auto slot_off = i * slot_stride;
        auto slot_span = pool_bytes.subspan(slot_off, slot_stride);
        write_i16_be(slot_span, VerticalEntitySweepsConfig::kVelYOffset, pool[i].vel_y);
    }

    return dy;
}

} // namespace oasis::collision
