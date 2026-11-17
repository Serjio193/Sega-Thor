#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace oasis::game::combat {

struct WeaponHitboxConfig {
    static constexpr std::uint32_t kFindFxPoolHitAddr = 0x00B9ECU;
    static constexpr std::uint32_t kFindNextFxPoolHitAddr = 0x00BA9CU;
    static constexpr std::uint32_t kBuildWeaponHitboxAllAddr = 0x00BAACU;
    static constexpr std::uint32_t kBuildWeaponHitboxEnemiesAddr = 0x00BAB8U;

    static constexpr std::uint32_t kFxPoolRam = 0x00FF2ABCU;
    static constexpr std::size_t kFxPoolSlotCount = 8U;
    static constexpr std::size_t kFxPoolStrideBytes = 0x005AU; // 90 bytes

    static constexpr std::uint32_t kFullEntityPoolRam = 0x00FF19E8U;
    static constexpr std::size_t kFullEntitySlotCount = 21U;
    static constexpr std::uint32_t kEnemyEntityPoolRam = 0x00FF1CD8U;
    static constexpr std::size_t kEnemyEntitySlotCount = 17U;
    static constexpr std::size_t kEnemyPoolStartIndex = 4U;
    static constexpr std::size_t kEntityStrideBytes = 0x00BCU; // 188 bytes

    static constexpr std::size_t kHitboxEntryBytes = 12U;
    static constexpr std::uint16_t kMirrorFacingId = 3U; // 0: Up, 1: Right, 2: Down, 3: Left (mirrors 1)
};

struct LocalHitbox {
    std::int16_t dz1{0};
    std::int16_t dz2{0};
    std::int16_t dx1{0};
    std::int16_t dy1{0};
    std::int16_t dx2{0};
    std::int16_t dy2{0};
};

struct WorldHitbox3D {
    std::int16_t x1{0};
    std::int16_t y1{0};
    std::int16_t x2{0};
    std::int16_t y2{0};
    std::int16_t z1{0};
    std::int16_t z2{0};
};

struct WeaponCallerEntity {
    std::int16_t pos_x{0};     // +0x08
    std::int16_t pos_y{0};     // +0x0C
    std::int16_t pos_z{0};     // +0x10
    std::int16_t elevation{0}; // +0x14
    std::uint16_t facing{0U};  // +0x16: 0: Up, 1: Right, 2: Down, 3: Left
};

struct FxEntity {
    std::int16_t state{0};        // +0x00: state (> 0 active, <= 0 inactive)
    std::int16_t pos_x{0};        // +0x08: X
    std::int16_t pos_y{0};        // +0x0C: Y
    std::int16_t pos_z{0};        // +0x10: Z
    std::int16_t elevation{0};    // +0x14: elevation bias
    std::int16_t half_width_x{0}; // +0x42: rx
    std::int16_t half_width_y{0}; // +0x44: ry
    std::int16_t height_z{0};     // +0x4A: hz
};

struct WeaponTargetEntity {
    std::int16_t state{0};        // +0x00: state (> 0 active, <= 0 inactive)
    std::int16_t pos_x{0};        // +0x08: X
    std::int16_t pos_y{0};        // +0x0C: Y
    std::int16_t pos_z{0};        // +0x10: Z
    std::int16_t elevation{0};    // +0x14: elevation bias
    std::uint8_t flags_38{0U};    // +0x38: bit 3 skips weapon hit
    std::uint8_t flags_3a{0U};    // +0x3A: bit 0 skips weapon hit
    std::int16_t half_width_x{0}; // +0x42: rx
    std::int16_t half_width_y{0}; // +0x44: ry
    std::int16_t height_z{0};     // +0x4A: hz
};

struct FxMatchResult {
    bool hit{false};
    std::size_t slot_index{0U};
    std::uint32_t ram_addr{0U};
};

struct FxIteratorState {
    std::size_t next_slot_index{0U};
    std::int16_t px1{0};
    std::int16_t py1{0};
    std::int16_t px2{0};
    std::int16_t py2{0};
    std::int16_t pz{0};
    bool active{false};
};

struct WeaponHitResult {
    bool hit{false};
    std::size_t slot_index{0U};
    std::uint32_t ram_addr{0U};
    WorldHitbox3D world_box{};
};

/// ROM 0x00BAC0..0x00BB16:
/// Transforms local hitbox with facing (facing == 3 mirrors X: dx1' = -dx2, dx2' = -dx1)
/// and translates by caller coordinates (ex, ey + y_off, ez + y_off).
[[nodiscard]] WorldHitbox3D build_world_weapon_hitbox(
    std::int16_t ex,
    std::int16_t ey,
    std::int16_t ez,
    std::int16_t y_off,
    std::uint16_t facing,
    const LocalHitbox& local_box) noexcept;

/// Indexes 3-entry facing table (0: Up, 1: Right, 2: Down, 3: Left uses table[1] with mirror).
[[nodiscard]] WorldHitbox3D build_world_weapon_hitbox_from_table(
    std::int16_t ex,
    std::int16_t ey,
    std::int16_t ez,
    std::int16_t y_off,
    std::uint16_t facing,
    std::span<const LocalHitbox, 3> table) noexcept;

/// ROM 0x00B9EC..0x00BA84:
/// Scans 8-slot FX pool ($FF2ABC) for 3D AABB overlap against probe box.
[[nodiscard]] FxMatchResult find_first_fx_pool_hit(
    const WeaponCallerEntity& caller,
    std::int16_t rx1,
    std::int16_t ry1,
    std::int16_t rx2,
    std::int16_t ry2,
    std::int16_t rz,
    std::span<const FxEntity> fx_slots,
    FxIteratorState& iter_state) noexcept;

/// Absolute coordinate version of FX-pool probe.
[[nodiscard]] FxMatchResult find_first_fx_pool_hit_abs(
    std::int16_t px1,
    std::int16_t py1,
    std::int16_t px2,
    std::int16_t py2,
    std::int16_t pz,
    std::span<const FxEntity> fx_slots,
    FxIteratorState& iter_state) noexcept;

/// ROM 0x00BA9C..0x00BAA8:
/// Resumes scanning remaining slots in the 8-slot FX pool.
[[nodiscard]] FxMatchResult find_next_fx_pool_hit(
    std::span<const FxEntity> fx_slots,
    FxIteratorState& iter_state) noexcept;

/// Scans candidate entity slots for collision with a world-space weapon hitbox.
[[nodiscard]] WeaponHitResult scan_entities_for_weapon_hit(
    std::size_t self_index,
    const WorldHitbox3D& world_box,
    std::span<const WeaponTargetEntity> entities,
    std::size_t start_slot,
    std::size_t slot_count) noexcept;

/// ROM 0x00BAAC..0x00BBA6:
/// Weapon hitbox check against full entity pool ($FF19E8).
[[nodiscard]] WeaponHitResult scan_weapon_hitbox_all(
    std::size_t self_index,
    const WeaponCallerEntity& caller,
    std::span<const LocalHitbox, 3> table,
    std::span<const WeaponTargetEntity> entities,
    std::size_t slot_count = WeaponHitboxConfig::kFullEntitySlotCount) noexcept;

/// ROM 0x00BAB8..0x00BBA6:
/// Weapon hitbox check against enemy entity pool ($FF1CD8, 17 slots).
[[nodiscard]] WeaponHitResult scan_weapon_hitbox_enemies(
    std::size_t self_index,
    const WeaponCallerEntity& caller,
    std::span<const LocalHitbox, 3> table,
    std::span<const WeaponTargetEntity> entities) noexcept;

} // namespace oasis::game::combat
