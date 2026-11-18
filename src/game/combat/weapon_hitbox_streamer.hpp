#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace oasis::game::combat {

/// Configuration and constants for ROM Weapon Attack Frame 3D Hitbox Streamer
/// & Dual-Pool Scanner Engine (0x00BAAC..0x00BC8E).
struct WeaponHitboxConfig {
    static constexpr std::uint32_t kPlayerEntryAddr = 0x00BAACU;
    static constexpr std::uint32_t kEnemiesEntryAddr = 0x00BAB8U;
    static constexpr std::uint32_t kPrimaryExitRtsAddr = 0x00BBA6U;
    static constexpr std::uint32_t kUnprotectedEntryAddr = 0x00BBA8U;
    static constexpr std::uint32_t kSecondaryExitRtsAddr = 0x00BC8CU;
    static constexpr std::uint32_t kWeaponHitboxRangeEnd = 0x00BC8EU;

    // RAM Pools
    static constexpr std::uint32_t kGroup2PoolRam = 0x00FF19E8U;
    static constexpr std::uint32_t kEnemyPoolRam = 0x00FF1CD8U;
    static constexpr std::size_t kPlayerSlotCount = 1U;
    static constexpr std::size_t kEnemySlotCount = 17U;
    static constexpr std::size_t kAllGroup2SlotCount = 21U;
    static constexpr std::size_t kSlotStride = 0xBCU; // 188 bytes

    // 12-byte stream format
    static constexpr std::size_t kHitboxEntryBytes = 12U;
    static constexpr std::size_t kWordsPerHitbox = 6U;

    // Entity slot field offsets
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

    // Target filter masks
    static constexpr std::uint8_t kInvulnerableMask = 0x08U; // bit 3
    static constexpr std::uint8_t kProtectedMask = 0x01U;    // bit 0
};

/// 12-byte raw hitbox record unpacked from ROM stream: [z_min, z_max, x_min, y_min, x_max, y_max].
struct RawHitboxRecord {
    std::int16_t z_min{0};
    std::int16_t z_max{0};
    std::int16_t x_min{0};
    std::int16_t y_min{0};
    std::int16_t x_max{0};
    std::int16_t y_max{0};

    /// Horizontal mirroring when facing left (status/facing == 3): neg x_min, neg x_max, exchange.
    [[nodiscard]] constexpr RawHitboxRecord mirror_horizontal() const noexcept {
        return RawHitboxRecord{
            z_min,
            z_max,
            static_cast<std::int16_t>(-x_max),
            y_min,
            static_cast<std::int16_t>(-x_min),
            y_max
        };
    }
};

/// 3D bounding box projected into world coordinates.
struct ProjectedHitbox {
    std::int16_t z_min{0};
    std::int16_t z_max{0};
    std::int16_t x_min{0};
    std::int16_t y_min{0};
    std::int16_t x_max{0};
    std::int16_t y_max{0};
};

/// Directional hitbox resolved from frame stream with mirroring flag.
struct DirectionalHitbox {
    RawHitboxRecord local_box{};
    bool mirrored{false};
    std::size_t stream_offset{0};
};

/// State of an entity acting as attacker.
struct AttackerEntityState {
    std::uint32_t address{0};
    std::int16_t world_x{0};
    std::int16_t world_y{0};
    std::int16_t elev_z{0};
    std::int16_t ground_h{0};
    std::uint16_t facing{0}; // 0=Down, 1=Right, 2=Up, 3=Left (mirrors Right)
};

/// State of an entity candidate for hit detection.
struct TargetEntityState {
    std::uint32_t address{0};
    std::int16_t type{0};
    std::int16_t world_x{0};
    std::int16_t world_y{0};
    std::int16_t elev_z{0};
    std::int16_t ground_h{0};
    std::uint8_t flags_38{0};
    std::uint8_t flags_3a{0};
    std::int16_t rad_x{0};
    std::int16_t rad_y{0};
    std::int16_t height_z{0};
};

/// Result of a weapon attack hitbox scan.
struct WeaponHitResult {
    bool carry{false};             // Carry flag: true if hit, false if miss
    std::uint32_t target_addr{0}; // ROM pointer in a0
    std::size_t slot_index{0};
    ProjectedHitbox projected_box{};
};

/// Unpacks a 12-byte raw hitbox record from big-endian bytes.
RawHitboxRecord unpack_hitbox_record(std::span<const std::uint8_t> stream, std::size_t byte_offset);

/// Resolves the directional 12-byte hitbox for a given facing direction.
/// Facing 0 (Down) -> offset 0
/// Facing 1 (Right) -> offset 12
/// Facing 2 (Up) -> offset 24
/// Facing 3 (Left) -> offset 12, mirrored horizontally (neg x_min, neg x_max, exchange)
DirectionalHitbox resolve_directional_hitbox(std::uint16_t facing, std::span<const std::uint8_t> stream);

/// Projects a local 12-byte hitbox into 3D world space.
ProjectedHitbox project_to_world(
    const RawHitboxRecord& local_box,
    std::int16_t world_x,
    std::int16_t world_y,
    std::int16_t elev_z,
    std::int16_t ground_h) noexcept;

/// Checks if a candidate target passes eligibility filters.
bool is_target_eligible(
    const TargetEntityState& target,
    std::uint32_t attacker_addr,
    bool ignore_protection = false) noexcept;

/// Tests 3D AABB overlap between the projected weapon hitbox and a target entity.
bool test_aabb_overlap(
    const ProjectedHitbox& box,
    const TargetEntityState& target) noexcept;

/// Scans a sequence of entity slots with the weapon hitbox.
WeaponHitResult scan_weapon_hitbox_pool(
    const AttackerEntityState& attacker,
    std::span<const std::uint8_t> hitbox_stream,
    std::span<const TargetEntityState> targets,
    bool ignore_protection = false);

/// Scans the player slot (0x00BAAC).
WeaponHitResult scan_player_weapon_hitbox(
    const AttackerEntityState& attacker,
    std::span<const std::uint8_t> hitbox_stream,
    const TargetEntityState& player_slot);

/// Scans enemy slots (0x00BAB8, up to 17 slots).
WeaponHitResult scan_enemy_weapon_hitbox(
    const AttackerEntityState& attacker,
    std::span<const std::uint8_t> hitbox_stream,
    std::span<const TargetEntityState> enemy_slots);

/// Scans enemy slots with protection ignored (0x00BBA8, up to 17 slots).
WeaponHitResult scan_enemy_weapon_hitbox_unprotected(
    const AttackerEntityState& attacker,
    std::span<const std::uint8_t> hitbox_stream,
    std::span<const TargetEntityState> enemy_slots);

/// Decodes target entity state from a 188-byte entity slot in 64KB RAM window.
TargetEntityState read_target_entity_raw(
    std::span<const std::uint8_t> ram,
    std::uint32_t slot_addr);

/// Decodes attacker entity state from a 188-byte entity slot in 64KB RAM window.
AttackerEntityState read_attacker_entity_raw(
    std::span<const std::uint8_t> ram,
    std::uint32_t slot_addr);

/// Raw 64KB RAM window scan for player weapon hitbox (0x00BAAC).
WeaponHitResult scan_player_weapon_hitbox_raw(
    std::span<const std::uint8_t> ram,
    std::uint32_t attacker_addr,
    std::span<const std::uint8_t> hitbox_stream);

/// Raw 64KB RAM window scan for enemy weapon hitbox (0x00BAB8).
WeaponHitResult scan_enemy_weapon_hitbox_raw(
    std::span<const std::uint8_t> ram,
    std::uint32_t attacker_addr,
    std::span<const std::uint8_t> hitbox_stream);

/// Raw 64KB RAM window scan for unprotected enemy weapon hitbox (0x00BBA8).
WeaponHitResult scan_enemy_weapon_hitbox_unprotected_raw(
    std::span<const std::uint8_t> ram,
    std::uint32_t attacker_addr,
    std::span<const std::uint8_t> hitbox_stream);

/// Raw 64KB RAM window scan for all 21 Group 2 slots starting from player ($FF19E8).
WeaponHitResult scan_all_group2_weapon_hitbox_raw(
    std::span<const std::uint8_t> ram,
    std::uint32_t attacker_addr,
    std::span<const std::uint8_t> hitbox_stream);

} // namespace oasis::game::combat
