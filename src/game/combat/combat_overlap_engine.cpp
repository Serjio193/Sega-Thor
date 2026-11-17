#include "game/combat/combat_overlap_engine.hpp"

namespace oasis::game::combat {

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

[[nodiscard]] inline std::uint32_t read_u32_be(std::span<const std::uint8_t> buf, std::size_t offset) noexcept {
    if (offset + 3 >= buf.size()) {
        return 0U;
    }
    return (static_cast<std::uint32_t>(buf[offset]) << 24) |
           (static_cast<std::uint32_t>(buf[offset + 1]) << 16) |
           (static_cast<std::uint32_t>(buf[offset + 2]) << 8) |
           static_cast<std::uint32_t>(buf[offset + 3]);
}

inline void write_i16_be(std::span<std::uint8_t> buf, std::size_t offset, std::int16_t val) noexcept {
    if (offset + 1 < buf.size()) {
        buf[offset] = static_cast<std::uint8_t>((static_cast<std::uint16_t>(val) >> 8) & 0xFFU);
        buf[offset + 1] = static_cast<std::uint8_t>(static_cast<std::uint16_t>(val) & 0xFFU);
    }
}

inline void write_u16_be(std::span<std::uint8_t> buf, std::size_t offset, std::uint16_t val) noexcept {
    if (offset + 1 < buf.size()) {
        buf[offset] = static_cast<std::uint8_t>((val >> 8) & 0xFFU);
        buf[offset + 1] = static_cast<std::uint8_t>(val & 0xFFU);
    }
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

void register_combat_hit(CombatEntityState& self, CombatEntityState& target) noexcept {
    target.owner_7e = self.owner_7e;
    target.hit_cooldown_82 = static_cast<std::uint8_t>(target.hit_cooldown_82 + 1U);
    self.hit_cooldown_82 = static_cast<std::uint8_t>(self.hit_cooldown_82 + 1U);
    target.hit_status_86 = 0x8001U;
    target.invuln_timer_83 = 0x0CU; // 12 frames
    target.hit_stun_9d = 0U;
    target.damage_84 = self.damage_84;

    // Apply standard directional knockback launch vectors (0x00B82C..0x00B840)
    target.vel_x = 0x0200;
    target.vel_y = 0x01C0;
    target.vel_z = 0x00020000;
}

CombatScanMatch scan_group2_aabb(
    const CombatEntityState& self,
    const DirectionalHitbox& rel_hitbox,
    std::span<CombatEntityState> group2_pool,
    CombatOverlapIterator& iter) noexcept {
    const std::int16_t px_min = static_cast<std::int16_t>(self.world_x + rel_hitbox.x_min);
    const std::int16_t px_max = static_cast<std::int16_t>(self.world_x + rel_hitbox.x_max);
    const std::int16_t py_min = static_cast<std::int16_t>(self.world_y + self.ground_h + rel_hitbox.y_min);
    const std::int16_t py_max = static_cast<std::int16_t>(self.world_y + self.ground_h + rel_hitbox.y_max);
    const std::int16_t pz = static_cast<std::int16_t>(self.elev_z + self.ground_h);

    const std::size_t total = std::min(group2_pool.size(), CombatOverlapConfig::kGroup2SlotCount);

    for (std::size_t i = iter.next_slot; i < total; ++i) {
        if ((iter.slot_mask & (1U << (i % 32U))) == 0U) {
            continue;
        }

        const auto& target = group2_pool[i];
        if (target.type <= 0) {
            continue;
        }
        if (target.ram_addr != 0U && target.ram_addr == self.ram_addr) {
            continue;
        }

        const std::int16_t tx_min = static_cast<std::int16_t>(target.world_x - target.rad_x);
        const std::int16_t tx_max = static_cast<std::int16_t>(target.world_x + target.rad_x);
        if (px_min > tx_max || px_max < tx_min) {
            continue;
        }

        const std::int16_t ty_base = static_cast<std::int16_t>(target.world_y + target.ground_h);
        const std::int16_t ty_min = static_cast<std::int16_t>(ty_base - target.rad_y);
        const std::int16_t ty_max = static_cast<std::int16_t>(ty_base + target.rad_y);
        if (py_min > ty_max || py_max < ty_min) {
            continue;
        }

        const std::int16_t tz_base = static_cast<std::int16_t>(target.elev_z + target.ground_h);
        const std::int16_t tz_max = static_cast<std::int16_t>(tz_base + target.height_z);
        if (pz < tz_base || pz > tz_max) {
            continue;
        }

        // Match found!
        iter.next_slot = i + 1U;
        return {true, i, target.ram_addr};
    }

    iter.next_slot = total;
    return {false, 0U, 0U};
}

CombatScanMatch scan_group2_interval(
    const CombatEntityState& self,
    const DirectionalHitbox& rel_hitbox,
    std::int16_t rel_z_min,
    std::int16_t rel_z_max,
    std::span<CombatEntityState> group2_pool,
    CombatOverlapIterator& iter) noexcept {
    const std::int16_t px_min = static_cast<std::int16_t>(self.world_x + rel_hitbox.x_min);
    const std::int16_t px_max = static_cast<std::int16_t>(self.world_x + rel_hitbox.x_max);
    const std::int16_t py_min = static_cast<std::int16_t>(self.world_y + self.ground_h + rel_hitbox.y_min);
    const std::int16_t py_max = static_cast<std::int16_t>(self.world_y + self.ground_h + rel_hitbox.y_max);

    const std::int16_t self_z_base = static_cast<std::int16_t>(self.elev_z + self.ground_h);
    const std::int16_t pz_min = static_cast<std::int16_t>(self_z_base + rel_z_min);
    const std::int16_t pz_max = static_cast<std::int16_t>(self_z_base + rel_z_max);

    const std::size_t total = std::min(group2_pool.size(), CombatOverlapConfig::kGroup2SlotCount);

    for (std::size_t i = iter.next_slot; i < total; ++i) {
        if ((iter.slot_mask & (1U << (i % 32U))) == 0U) {
            continue;
        }

        const auto& target = group2_pool[i];
        if (target.type <= 0) {
            continue;
        }
        if (target.ram_addr != 0U && target.ram_addr == self.ram_addr) {
            continue;
        }

        const std::int16_t tx_min = static_cast<std::int16_t>(target.world_x - target.rad_x);
        const std::int16_t tx_max = static_cast<std::int16_t>(target.world_x + target.rad_x);
        if (px_min > tx_max || px_max < tx_min) {
            continue;
        }

        const std::int16_t ty_base = static_cast<std::int16_t>(target.world_y + target.ground_h);
        const std::int16_t ty_min = static_cast<std::int16_t>(ty_base - target.rad_y);
        const std::int16_t ty_max = static_cast<std::int16_t>(ty_base + target.rad_y);
        if (py_min > ty_max || py_max < ty_min) {
            continue;
        }

        const std::int16_t tz_base = static_cast<std::int16_t>(target.elev_z + target.ground_h);
        const std::int16_t tz_max = static_cast<std::int16_t>(tz_base + target.height_z);
        if (tz_max < pz_min || tz_base > pz_max) {
            continue;
        }

        iter.next_slot = i + 1U;
        return {true, i, target.ram_addr};
    }

    iter.next_slot = total;
    return {false, 0U, 0U};
}

CombatScanMatch scan_group4_breakables(
    const CombatEntityState& self,
    const DirectionalHitbox& rel_hitbox,
    std::span<CombatEntityState> group4_pool,
    CombatOverlapIterator& iter) noexcept {
    const std::int16_t px_min = static_cast<std::int16_t>(self.world_x + rel_hitbox.x_min);
    const std::int16_t px_max = static_cast<std::int16_t>(self.world_x + rel_hitbox.x_max);
    const std::int16_t py_min = static_cast<std::int16_t>(self.world_y + self.ground_h + rel_hitbox.y_min);
    const std::int16_t py_max = static_cast<std::int16_t>(self.world_y + self.ground_h + rel_hitbox.y_max);
    const std::int16_t pz = static_cast<std::int16_t>(self.elev_z + self.ground_h);

    const std::size_t total = std::min(group4_pool.size(), CombatOverlapConfig::kGroup4SlotCount);

    for (std::size_t i = iter.next_slot; i < total; ++i) {
        const auto& target = group4_pool[i];
        if (target.type <= 0) {
            continue;
        }

        const std::int16_t tx_min = static_cast<std::int16_t>(target.world_x - target.rad_x);
        const std::int16_t tx_max = static_cast<std::int16_t>(target.world_x + target.rad_x);
        if (px_min > tx_max || px_max < tx_min) {
            continue;
        }

        const std::int16_t ty_base = static_cast<std::int16_t>(target.world_y + target.ground_h);
        const std::int16_t ty_min = static_cast<std::int16_t>(ty_base - target.rad_y);
        const std::int16_t ty_max = static_cast<std::int16_t>(ty_base + target.rad_y);
        if (py_min > ty_max || py_max < ty_min) {
            continue;
        }

        const std::int16_t tz_base = static_cast<std::int16_t>(target.elev_z + target.ground_h);
        const std::int16_t tz_max = static_cast<std::int16_t>(tz_base + target.height_z);
        if (pz < tz_base || pz > tz_max) {
            continue;
        }

        iter.next_slot = i + 1U;
        return {true, i, target.ram_addr};
    }

    iter.next_slot = total;
    return {false, 0U, 0U};
}

std::size_t execute_directional_combat_attack(
    CombatEntityState& self,
    std::span<CombatEntityState> group2_pool) noexcept {
    const auto rel_hitbox = compute_directional_hitbox(self.rad_x, self.rad_y, self.facing);
    CombatOverlapIterator iter{0xFFFFFFFFU, 0U, true};

    std::size_t hits = 0U;
    while (true) {
        auto match = scan_group2_aabb(self, rel_hitbox, group2_pool, iter);
        if (!match.hit) {
            break;
        }

        auto& target = group2_pool[match.slot_index];
        if (is_target_vulnerable(target.flags_38, target.flags_3a, target.hit_cooldown_82,
                                 self.owner_7e, target.ram_addr)) {
            register_combat_hit(self, target);
            hits += 1U;
        }
    }

    return hits;
}

std::size_t scan_combat_overlap_raw(
    std::span<std::uint8_t> ram_buffer,
    std::uint32_t self_addr,
    std::uint32_t ram_base_addr) {
    auto ram_offset = [ram_base_addr, buf_size = ram_buffer.size()](std::uint32_t addr) -> std::size_t {
        if (addr < ram_base_addr || (addr - ram_base_addr) >= buf_size) {
            return buf_size;
        }
        return static_cast<std::size_t>(addr - ram_base_addr);
    };

    const std::size_t self_pos = ram_offset(self_addr);
    if (self_pos + CombatOverlapConfig::kGroup2SlotStride > ram_buffer.size()) {
        return 0U;
    }

    CombatEntityState self{};
    self.type = read_i16_be(ram_buffer, self_pos + CombatOverlapConfig::kTypeOffset);
    self.world_x = read_i16_be(ram_buffer, self_pos + CombatOverlapConfig::kWorldXOffset);
    self.world_y = read_i16_be(ram_buffer, self_pos + CombatOverlapConfig::kWorldYOffset);
    self.elev_z = read_i16_be(ram_buffer, self_pos + CombatOverlapConfig::kElevZOffset);
    self.ground_h = read_i16_be(ram_buffer, self_pos + CombatOverlapConfig::kGroundHOffset);
    self.facing = read_u16_be(ram_buffer, self_pos + CombatOverlapConfig::kFacingOffset);
    self.rad_x = read_i16_be(ram_buffer, self_pos + CombatOverlapConfig::kRadiusXOffset);
    self.rad_y = read_i16_be(ram_buffer, self_pos + CombatOverlapConfig::kRadiusYOffset);
    self.damage_84 = read_i16_be(ram_buffer, self_pos + CombatOverlapConfig::kDamageOffset);
    self.owner_7e = read_u32_be(ram_buffer, self_pos + CombatOverlapConfig::kOwnerOffset);
    self.hit_cooldown_82 = ram_buffer[self_pos + CombatOverlapConfig::kHitCooldownOffset];
    self.ram_addr = self_addr;

    std::array<CombatEntityState, CombatOverlapConfig::kGroup2SlotCount> targets{};
    const std::size_t pool_pos = ram_offset(CombatOverlapConfig::kGroup2PoolRam);

    for (std::size_t i = 0; i < CombatOverlapConfig::kGroup2SlotCount; ++i) {
        const std::size_t slot_pos = pool_pos + i * CombatOverlapConfig::kGroup2SlotStride;
        if (slot_pos + CombatOverlapConfig::kGroup2SlotStride > ram_buffer.size()) {
            break;
        }

        auto& t = targets[i];
        t.type = read_i16_be(ram_buffer, slot_pos + CombatOverlapConfig::kTypeOffset);
        t.world_x = read_i16_be(ram_buffer, slot_pos + CombatOverlapConfig::kWorldXOffset);
        t.world_y = read_i16_be(ram_buffer, slot_pos + CombatOverlapConfig::kWorldYOffset);
        t.elev_z = read_i16_be(ram_buffer, slot_pos + CombatOverlapConfig::kElevZOffset);
        t.ground_h = read_i16_be(ram_buffer, slot_pos + CombatOverlapConfig::kGroundHOffset);
        t.flags_38 = ram_buffer[slot_pos + CombatOverlapConfig::kFlags38Offset];
        t.flags_3a = ram_buffer[slot_pos + CombatOverlapConfig::kFlags3AOffset];
        t.rad_x = read_i16_be(ram_buffer, slot_pos + CombatOverlapConfig::kRadiusXOffset);
        t.rad_y = read_i16_be(ram_buffer, slot_pos + CombatOverlapConfig::kRadiusYOffset);
        t.height_z = read_i16_be(ram_buffer, slot_pos + CombatOverlapConfig::kHeightZOffset);
        t.owner_7e = read_u32_be(ram_buffer, slot_pos + CombatOverlapConfig::kOwnerOffset);
        t.hit_cooldown_82 = ram_buffer[slot_pos + CombatOverlapConfig::kHitCooldownOffset];
        t.ram_addr = static_cast<std::uint32_t>(CombatOverlapConfig::kGroup2PoolRam + i * CombatOverlapConfig::kGroup2SlotStride);
    }

    const std::size_t hit_count = execute_directional_combat_attack(self, targets);

    // Write back self
    ram_buffer[self_pos + CombatOverlapConfig::kHitCooldownOffset] = self.hit_cooldown_82;

    // Write back modified targets
    for (std::size_t i = 0; i < CombatOverlapConfig::kGroup2SlotCount; ++i) {
        const auto& t = targets[i];
        const std::size_t slot_pos = pool_pos + i * CombatOverlapConfig::kGroup2SlotStride;
        if (slot_pos + CombatOverlapConfig::kGroup2SlotStride <= ram_buffer.size()) {
            write_u32_be(ram_buffer, slot_pos + CombatOverlapConfig::kOwnerOffset, t.owner_7e);
            ram_buffer[slot_pos + CombatOverlapConfig::kHitCooldownOffset] = t.hit_cooldown_82;
            ram_buffer[slot_pos + CombatOverlapConfig::kInvulnTimerOffset] = t.invuln_timer_83;
            write_i16_be(ram_buffer, slot_pos + CombatOverlapConfig::kDamageOffset, t.damage_84);
            write_u16_be(ram_buffer, slot_pos + CombatOverlapConfig::kHitStatusOffset, t.hit_status_86);
            ram_buffer[slot_pos + CombatOverlapConfig::kHitStunOffset] = t.hit_stun_9d;
            write_i16_be(ram_buffer, slot_pos + CombatOverlapConfig::kVelXOffset, static_cast<std::int16_t>(t.vel_x));
            write_i16_be(ram_buffer, slot_pos + CombatOverlapConfig::kVelYOffset, static_cast<std::int16_t>(t.vel_y));
            write_u32_be(ram_buffer, slot_pos + CombatOverlapConfig::kVelZOffset, static_cast<std::uint32_t>(t.vel_z));
        }
    }

    return hit_count;
}

} // namespace oasis::game::combat
