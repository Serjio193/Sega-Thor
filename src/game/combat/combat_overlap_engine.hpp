#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace oasis::game::combat {

/// Configuration and constants for ROM Entity 3D AABB Combat Overlap & Multi-Pool Collision Iterator Engine (0x00B79A..0x00BAAC).
struct CombatOverlapConfig {
    static constexpr std::uint32_t kDirectionalHitboxScannerAddr = 0x00B79AU;
    static constexpr std::uint32_t kDirectionalScannerExitRtsAddr = 0x00B850U;
    static constexpr std::uint32_t kGroup2AabbScannerAddr = 0x00B856U;
    static constexpr std::uint32_t kGroup2AabbHitExitRtsAddr = 0x00B910U;
    static constexpr std::uint32_t kGroup2AabbResumeAddr = 0x00B912U;
    static constexpr std::uint32_t kGroup2IntervalScannerAddr = 0x00B922U;
    static constexpr std::uint32_t kGroup2IntervalHitExitRtsAddr = 0x00B9DCU;
    static constexpr std::uint32_t kGroup2IntervalResumeAddr = 0x00B9DEU;
    static constexpr std::uint32_t kGroup4ScannerAddr = 0x00B9ECU;
    static constexpr std::uint32_t kGroup4HitExitRtsAddr = 0x00BA9AU;
    static constexpr std::uint32_t kGroup4ResumeAddr = 0x00BA9CU;
    static constexpr std::uint32_t kCombatOverlapRangeEnd = 0x00BAACU;

    // RAM Pools
    static constexpr std::uint32_t kGroup2PoolRam = 0x00FF19E8U;
    static constexpr std::size_t kGroup2SlotCount = 21U;
    static constexpr std::size_t kGroup2SlotStride = 0xBCU; // 188 bytes

    static constexpr std::uint32_t kGroup4PoolRam = 0x00FF2ABCU;
    static constexpr std::size_t kGroup4SlotCount = 8U;
    static constexpr std::size_t kGroup4SlotStride = 0x5AU; // 90 bytes

    static constexpr std::uint32_t kContextSaveBufferRam = 0x00FF194AU;
    static constexpr std::uint32_t kContextSaveBufferEndRam = 0x00FF196AU;

    // Slot field offsets
    static constexpr std::size_t kTypeOffset = 0x00U;
    static constexpr std::size_t kWorldXOffset = 0x08U;
    static constexpr std::size_t kWorldYOffset = 0x0CU;
    static constexpr std::size_t kElevZOffset = 0x10U;
    static constexpr std::size_t kGroundHOffset = 0x14U;
    static constexpr std::size_t kFacingOffset = 0x16U;
    static constexpr std::size_t kFlags38Offset = 0x38U;
    static constexpr std::size_t kFlags3AOffset = 0x3AU;
    static constexpr std::size_t kRadiusXOffset = 0x42U;
    static constexpr std::size_t kRadiusYOffset = 0x44U;
    static constexpr std::size_t kHeightZOffset = 0x4AU;
    static constexpr std::size_t kVelXOffset = 0x4EU;
    static constexpr std::size_t kVelYOffset = 0x52U;
    static constexpr std::size_t kVelZOffset = 0x56U;
    static constexpr std::size_t kOwnerOffset = 0x7EU;
    static constexpr std::size_t kHitCooldownOffset = 0x82U;
    static constexpr std::size_t kInvulnTimerOffset = 0x83U;
    static constexpr std::size_t kDamageOffset = 0x84U;
    static constexpr std::size_t kHitStatusOffset = 0x86U;
    static constexpr std::size_t kHitStunOffset = 0x9DU;
};

/// 2D Directional Hitbox relative to entity origin.
struct DirectionalHitbox {
    std::int16_t x_min{0};
    std::int16_t x_max{0};
    std::int16_t y_min{0};
    std::int16_t y_max{0};

    [[nodiscard]] constexpr bool operator==(const DirectionalHitbox&) const noexcept = default;
};

/// 3D Axis-Aligned Bounding Box in world coordinates.
struct CombatAabb3D {
    std::int16_t x_min{0};
    std::int16_t x_max{0};
    std::int16_t y_min{0};
    std::int16_t y_max{0};
    std::int16_t z_min{0};
    std::int16_t z_max{0};

    [[nodiscard]] constexpr bool overlaps(const CombatAabb3D& other) const noexcept {
        return (x_max >= other.x_min) && (x_min <= other.x_max) &&
               (y_max >= other.y_min) && (y_min <= other.y_max) &&
               (z_max >= other.z_min) && (z_min <= other.z_max);
    }

    [[nodiscard]] constexpr bool contains_z(std::int16_t z) const noexcept {
        return (z >= z_min) && (z <= z_max);
    }
};

/// High-level combat entity representation for overlap tests.
struct CombatEntityState {
    std::int16_t type{0};
    std::int16_t world_x{0};
    std::int16_t world_y{0};
    std::int16_t elev_z{0};
    std::int16_t ground_h{0};
    std::uint16_t facing{0U};
    std::uint8_t flags_38{0U};
    std::uint8_t flags_3a{0U};
    std::int16_t rad_x{0};
    std::int16_t rad_y{0};
    std::int16_t height_z{0};
    std::int32_t vel_x{0};
    std::int32_t vel_y{0};
    std::int32_t vel_z{0};
    std::uint32_t owner_7e{0U};
    std::uint8_t hit_cooldown_82{0U};
    std::uint8_t invuln_timer_83{0U};
    std::int16_t damage_84{0};
    std::uint16_t hit_status_86{0U};
    std::uint8_t hit_stun_9d{0U};
    std::uint32_t ram_addr{0U};
};

/// Computes forward-biased directional hitbox based on 4-way facing (0x00B7AC..0x00B7C8).
[[nodiscard]] constexpr DirectionalHitbox compute_directional_hitbox(
    std::int16_t rad_x,
    std::int16_t rad_y,
    std::uint16_t facing) noexcept {
    DirectionalHitbox box{
        static_cast<std::int16_t>(-rad_x),
        rad_x,
        static_cast<std::int16_t>(-rad_y),
        rad_y
    };
    switch (facing & 0x03U) {
        case 0U: box.y_min = 0; break; // Down: d2 cleared
        case 1U: box.x_max = 0; break; // Left: d3 cleared
        case 2U: box.y_max = 0; break; // Up: d4 cleared
        case 3U: box.x_min = 0; break; // Right: d1 cleared
        default: break;
    }
    return box;
}

/// Evaluates combat target vulnerability (0x00B7D2..0x00B7F6).
[[nodiscard]] constexpr bool is_target_vulnerable(
    std::uint8_t flags_38,
    std::uint8_t flags_3a,
    std::uint8_t hit_cooldown_82,
    std::uint32_t self_owner_7e,
    std::uint32_t target_addr) noexcept {
    if ((flags_38 & 0x0CU) != 0U) {
        return false; // Intangible / invulnerable
    }
    if ((flags_3a & 0x01U) != 0U) {
        return false; // Protected
    }
    if (hit_cooldown_82 != 0U) {
        return false; // Hit cooldown active
    }
    if (self_owner_7e != 0U && target_addr != 0U && self_owner_7e == target_addr) {
        return false; // Self damage prevention
    }
    return true;
}

/// Computes target 3D world AABB bounds.
[[nodiscard]] constexpr CombatAabb3D compute_entity_world_aabb(const CombatEntityState& ent) noexcept {
    CombatAabb3D box{};
    box.x_min = static_cast<std::int16_t>(ent.world_x - ent.rad_x);
    box.x_max = static_cast<std::int16_t>(ent.world_x + ent.rad_x);
    const std::int16_t gy = static_cast<std::int16_t>(ent.world_y + ent.ground_h);
    box.y_min = static_cast<std::int16_t>(gy - ent.rad_y);
    box.y_max = static_cast<std::int16_t>(gy + ent.rad_y);
    const std::int16_t zb = static_cast<std::int16_t>(ent.elev_z + ent.ground_h);
    box.z_min = zb;
    box.z_max = static_cast<std::int16_t>(zb + ent.height_z);
    return box;
}

/// Applies hit registration and knockback to target (0x00B7FA..0x00B840).
void register_combat_hit(CombatEntityState& self, CombatEntityState& target) noexcept;

/// Result of a multi-pool combat overlap scan.
struct CombatScanMatch {
    bool hit{false};
    std::size_t slot_index{0U};
    std::uint32_t target_addr{0U};
};

/// Iterator for resuming multi-pool collision scans.
struct CombatOverlapIterator {
    std::uint32_t slot_mask{0xFFFFFFFFU};
    std::size_t next_slot{0U};
    bool active{false};
};

/// Scans Group 2 pool (21 slots) for 3D AABB point overlap (0x00B856..0x00B920).
[[nodiscard]] CombatScanMatch scan_group2_aabb(
    const CombatEntityState& self,
    const DirectionalHitbox& rel_hitbox,
    std::span<CombatEntityState> group2_pool,
    CombatOverlapIterator& iter) noexcept;

/// Scans Group 2 pool (21 slots) for 3D AABB interval overlap (0x00B922..0x00B9DC).
[[nodiscard]] CombatScanMatch scan_group2_interval(
    const CombatEntityState& self,
    const DirectionalHitbox& rel_hitbox,
    std::int16_t rel_z_min,
    std::int16_t rel_z_max,
    std::span<CombatEntityState> group2_pool,
    CombatOverlapIterator& iter) noexcept;

/// Scans Group 4 pool (8 slots) for breakable/pickup collisions (0x00B9EC..0x00BAAC).
[[nodiscard]] CombatScanMatch scan_group4_breakables(
    const CombatEntityState& self,
    const DirectionalHitbox& rel_hitbox,
    std::span<CombatEntityState> group4_pool,
    CombatOverlapIterator& iter) noexcept;

/// High-level directional combat attack routine executing the full loop (0x00B79A..0x00B850).
/// Returns number of valid hits delivered.
std::size_t execute_directional_combat_attack(
    CombatEntityState& self,
    std::span<CombatEntityState> group2_pool) noexcept;

/// Raw 64KB RAM window integration for combat overlap scans.
[[nodiscard]] std::size_t scan_combat_overlap_raw(
    std::span<std::uint8_t> ram_buffer,
    std::uint32_t self_addr,
    std::uint32_t ram_base_addr = 0x00FF0000U);

} // namespace oasis::game::combat
