#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::game::combat {

/// Configuration and constants for ROM Combat Hit Spark Particle Generator
/// & 3D Overlap Centerpoint Calculator (0x00C012..0x00C0BE, 172 bytes).
struct HitSparkGeneratorConfig {
    static constexpr std::uint32_t kHitSparkGeneratorAddr = 0x00C012U;
    static constexpr std::uint32_t kHitSparkInitAddr = 0x00C038U;
    static constexpr std::uint32_t kExitRtsAddr = 0x00C0BCU;
    static constexpr std::uint32_t kRangeEndAddr = 0x00C0BEU;
    static constexpr std::size_t kRoutineSizeBytes = 172U;

    // Direct callers in combat engine
    static constexpr std::uint32_t kUnblockedHitCallerAddr = 0x00BEFEU;
    static constexpr std::uint32_t kBlockedHitCallerAddr = 0x00BFF6U;

    // RAM Ring Buffer ($FF19C8..$FF19E8)
    static constexpr std::uint32_t kSparkRingBufferRam = 0x00FF19C8U;
    static constexpr std::size_t kSlotCount = 4U;
    static constexpr std::size_t kSlotSizeBytes = 8U;
    static constexpr std::uint16_t kSparkType = 2U;
    static constexpr std::uint16_t kDefaultLifetime = 5U;

    // Slot field offsets (8 bytes per particle)
    static constexpr std::size_t kSparkTypeOffset = 0x00U;
    static constexpr std::size_t kSparkXOffset = 0x02U;
    static constexpr std::size_t kSparkYOffset = 0x04U;
    static constexpr std::size_t kSparkLifetimeOffset = 0x06U;

    // Target entity field offsets
    static constexpr std::size_t kTargetWorldXOffset = 0x08U;
    static constexpr std::size_t kTargetWorldYOffset = 0x0CU;
    static constexpr std::size_t kTargetElevZOffset = 0x10U;
    static constexpr std::size_t kTargetGroundHOffset = 0x14U;
    static constexpr std::size_t kTargetRadXOffset = 0x42U;
    static constexpr std::size_t kTargetRadYOffset = 0x44U;
    static constexpr std::size_t kTargetHeightZOffset = 0x4AU;
};

/// 3D Attack Hitbox (world coordinates) passed in registers:
/// a1: z_min, d7: z_max, d1: x_min, d3: x_max, d2: y_min, d4: y_max.
struct AttackHitbox3D {
    std::int16_t z_min{0};
    std::int16_t z_max{0};
    std::int16_t x_min{0};
    std::int16_t y_min{0};
    std::int16_t x_max{0};
    std::int16_t y_max{0};

    [[nodiscard]] constexpr bool operator==(const AttackHitbox3D&) const noexcept = default;
};

/// Target entity state required for 3D overlap midpoint projection.
struct SparkTargetState {
    std::uint32_t address{0U};
    std::int16_t world_x{0};
    std::int16_t world_y{0};
    std::int16_t elev_z{0};
    std::int16_t ground_h{0};
    std::int16_t rad_x{0};
    std::int16_t rad_y{0};
    std::int16_t height_z{0};

    [[nodiscard]] constexpr bool operator==(const SparkTargetState&) const noexcept = default;
};

/// Single particle record in the 4-slot ring buffer ($FF19C8..$FF19E8).
struct HitSparkParticle {
    std::uint16_t type{0U};      // 0 = free, 2 = active
    std::int16_t x{0};           // mid_x
    std::int16_t y{0};           // mid_y - mid_z (elevation subtracted!)
    std::uint16_t lifetime{0U};  // 5 frames

    [[nodiscard]] constexpr bool is_active() const noexcept {
        return (type != 0U) && (lifetime > 0U);
    }

    [[nodiscard]] constexpr bool operator==(const HitSparkParticle&) const noexcept = default;
};

/// 3D impact midpoint volume and 2D projected screen coordinate.
struct SparkMidpointResult {
    std::int16_t mid_z{0};
    std::int16_t mid_x{0};
    std::int16_t mid_y{0};
    std::int16_t projected_x{0}; // spark_x = mid_x
    std::int16_t projected_y{0}; // spark_y = mid_y - mid_z

    [[nodiscard]] constexpr bool operator==(const SparkMidpointResult&) const noexcept = default;
};

/// Result of attempting to allocate and spawn a hit spark particle.
struct SpawnSparkResult {
    bool spawned{false};
    std::size_t slot_index{0U};
    SparkMidpointResult midpoint{};
    HitSparkParticle particle{};

    [[nodiscard]] constexpr bool operator==(const SpawnSparkResult&) const noexcept = default;
};

/// Computes the 3D intersection volume midpoint and projected 2D coordinates (0x00C046..0x00C0B6).
[[nodiscard]] constexpr SparkMidpointResult compute_spark_midpoints(
    const AttackHitbox3D& hitbox,
    const SparkTargetState& target) noexcept {
    // 1. Z Midpoint (0x00C046..0x00C068)
    const auto tz_min = static_cast<std::int16_t>(target.elev_z + target.ground_h);
    const auto tz_max = static_cast<std::int16_t>(tz_min + target.height_z);
    const std::int16_t z0 = (hitbox.z_min >= tz_min) ? hitbox.z_min : tz_min;
    const std::int16_t z1 = (hitbox.z_max <= tz_max) ? hitbox.z_max : tz_max;
    const auto mid_z = static_cast<std::int16_t>(z0 + ((z1 - z0) >> 1));

    // 2. X Midpoint (0x00C06A..0x00C088)
    const auto tx_min = static_cast<std::int16_t>(target.world_x - target.rad_x);
    const auto tx_max = static_cast<std::int16_t>(target.world_x + target.rad_x);
    const std::int16_t x0 = (hitbox.x_min >= tx_min) ? hitbox.x_min : tx_min;
    const std::int16_t x1 = (hitbox.x_max <= tx_max) ? hitbox.x_max : tx_max;
    const auto mid_x = static_cast<std::int16_t>(x0 + ((x1 - x0) >> 1));

    // 3. Y Midpoint (0x00C08A..0x00C0AC)
    const auto ty_base = static_cast<std::int16_t>(target.world_y + target.ground_h);
    const auto ty_min = static_cast<std::int16_t>(ty_base - target.rad_y);
    const auto ty_max = static_cast<std::int16_t>(ty_base + target.rad_y);
    const std::int16_t y0 = (hitbox.y_min >= ty_min) ? hitbox.y_min : ty_min;
    const std::int16_t y1 = (hitbox.y_max <= ty_max) ? hitbox.y_max : ty_max;
    const auto mid_y = static_cast<std::int16_t>(y0 + ((y1 - y0) >> 1));

    // 4. Projected Screen Position (0x00C0AE..0x00C0B6): Y elevation offset subtracted!
    const std::int16_t projected_x = mid_x;
    const auto projected_y = static_cast<std::int16_t>(mid_y - mid_z);

    return SparkMidpointResult{
        .mid_z = mid_z,
        .mid_x = mid_x,
        .mid_y = mid_y,
        .projected_x = projected_x,
        .projected_y = projected_y
    };
}

/// Spawns a hit spark into the first free slot of the 4-slot ring buffer (0x00C012..0x00C0BE).
[[nodiscard]] SpawnSparkResult spawn_hit_spark(
    const AttackHitbox3D& hitbox,
    const SparkTargetState& target,
    std::span<HitSparkParticle, 4> slots) noexcept;

/// Advances lifetimes of active hit spark particles by 1 frame.
void update_hit_sparks(std::span<HitSparkParticle, 4> slots) noexcept;

/// High-level particle manager handling the 4-slot spark ring buffer.
class HitSparkManager {
public:
    static constexpr std::size_t kMaxSparks = HitSparkGeneratorConfig::kSlotCount;
    static constexpr std::uint16_t kSparkType = HitSparkGeneratorConfig::kSparkType;
    static constexpr std::uint16_t kDefaultLifetime = HitSparkGeneratorConfig::kDefaultLifetime;

    [[nodiscard]] SpawnSparkResult spawn_spark(
        const AttackHitbox3D& hitbox,
        const SparkTargetState& target) noexcept {
        return spawn_hit_spark(hitbox, target, m_sparks);
    }

    void update_frame() noexcept {
        update_hit_sparks(m_sparks);
    }

    [[nodiscard]] std::span<const HitSparkParticle, 4> sparks() const noexcept {
        return m_sparks;
    }

    [[nodiscard]] std::span<HitSparkParticle, 4> sparks() noexcept {
        return m_sparks;
    }

    void reset() noexcept {
        m_sparks.fill(HitSparkParticle{});
    }

private:
    std::array<HitSparkParticle, 4> m_sparks{};
};

// ============================================================================
// Raw 64KB RAM Window Integration Helpers
// ============================================================================

[[nodiscard]] HitSparkParticle read_hit_spark_particle_raw(
    std::span<const std::uint8_t> ram,
    std::size_t slot_index);

void write_hit_spark_particle_raw(
    std::span<std::uint8_t> ram,
    std::size_t slot_index,
    const HitSparkParticle& particle);

[[nodiscard]] SparkTargetState read_spark_target_raw(
    std::span<const std::uint8_t> ram,
    std::uint32_t target_addr);

[[nodiscard]] SpawnSparkResult spawn_hit_spark_raw(
    std::span<std::uint8_t> ram,
    const AttackHitbox3D& hitbox,
    std::uint32_t target_addr);

} // namespace oasis::game::combat
