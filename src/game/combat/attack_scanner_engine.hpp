#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <vector>

namespace oasis::game::combat {

/// Configuration and constants for ROM Universal Attack Frame Scanner &
/// 3D Hitbox Intersection Engine (0x00BC8E..0x00BE98).
struct AttackScannerConfig {
    static constexpr std::uint32_t kScanAllEntitiesAddr = 0x00BC8EU;
    static constexpr std::uint32_t kScanGroup3AAddr = 0x00BCA4U;
    static constexpr std::uint32_t kScanGroup3BAddr = 0x00BCBAU;
    static constexpr std::uint32_t kScanSingleAddr = 0x00BCD8U;
    static constexpr std::uint32_t kExitRtsAddr = 0x00BE96U;
    static constexpr std::uint32_t kRangeEndAddr = 0x00BE98U;

    static constexpr std::uint32_t kCallbackGroup3Addr = 0x00BE98U;
    static constexpr std::uint32_t kCallbackAllAddr = 0x00BEBAU;
    static constexpr std::uint32_t kDurabilityHookAddr = 0x008530U;

    // RAM Addresses
    static constexpr std::uint32_t kCallbackPtrRam = 0x00FF1940U;
    static constexpr std::uint32_t kModeFlag198BRam = 0x00FF198BU;
    static constexpr std::uint32_t kAttackType198CRam = 0x00FF198CU;
    static constexpr std::uint32_t kPlayerEntityRam = 0x00FF19E8U;
    static constexpr std::uint32_t kGroup3EntityRam = 0x00FF1CD8U;

    // Pool Slot Counts and Stride
    static constexpr std::size_t kAllEntitiesSlotCount = 21U;
    static constexpr std::size_t kGroup3SlotCount = 17U;
    static constexpr std::size_t kSingleSlotCount = 1U;
    static constexpr std::size_t kSlotStride = 0xBCU; // 188 bytes

    // Record Layout (0x2C = 44 bytes)
    static constexpr std::size_t kRecordSize = 44U;
    static constexpr std::size_t kHitboxesOffset = 8U;
    static constexpr std::int16_t kTerminatorFrame = -1; // 0xFFFF

    // Attacker Flags
    static constexpr std::uint8_t kHitRegisteredMask = 0x08U; // bit 3 on +0x37
    static constexpr std::uint8_t kInvulnerableMask = 0x08U;    // bit 3 on +0x38

    // Slot Field Offsets
    static constexpr std::size_t kTypeOffset = 0x00U;
    static constexpr std::size_t kWorldXOffset = 0x08U;
    static constexpr std::size_t kWorldYOffset = 0x0CU;
    static constexpr std::size_t kElevZOffset = 0x10U;
    static constexpr std::size_t kGroundHOffset = 0x14U;
    static constexpr std::size_t kFacingOffset = 0x16U;
    static constexpr std::size_t kAnimFrameOffset = 0x32U;
    static constexpr std::size_t kFlags37Offset = 0x37U;
    static constexpr std::size_t kFlags38Offset = 0x38U;
    static constexpr std::size_t kRadiusXOffset = 0x42U;
    static constexpr std::size_t kRadiusYOffset = 0x44U;
    static constexpr std::size_t kHeightZOffset = 0x4AU;
    static constexpr std::size_t kHitCooldownOffset = 0x82U;
    static constexpr std::size_t kWeaponBonus90Offset = 0x90U;
    static constexpr std::size_t kStrengthStat96Offset = 0x96U;
};

/// Strength threshold progression for Ali's strength rank bonus.
struct StrengthThreshold {
    std::uint16_t threshold;
    std::uint16_t bonus;
};

inline constexpr std::array<StrengthThreshold, 7> kAliStrengthThresholds{{
    {10U, 1U},
    {20U, 1U},
    {30U, 1U},
    {40U, 1U},
    {50U, 1U},
    {100U, 2U},
    {200U, 4U}
}};

/// Calculates cumulative strength rank bonus for Ali from stat +0x96.
[[nodiscard]] constexpr std::uint16_t calculate_ali_strength_bonus(std::uint16_t stat_96) noexcept {
    std::uint16_t bonus = 0U;
    for (const auto& entry : kAliStrengthThresholds) {
        if (stat_96 >= entry.threshold) {
            bonus += entry.bonus;
        }
    }
    return bonus;
}

/// Packed attack damage and stun parameters stored into register a4.
struct AttackDamageDesc {
    std::uint16_t total_damage{0};
    std::uint8_t stun_duration{0};
    std::uint32_t packed_a4{0};
};

/// Packs base damage, element, stun duration, strength rank bonus, and weapon bonus into a4.
[[nodiscard]] constexpr AttackDamageDesc pack_attack_damage(
    std::uint8_t base_dmg,
    std::uint8_t element_raw,
    std::uint16_t stat_96,
    std::uint16_t weapon_bonus_90,
    bool is_ali) noexcept {
    std::uint16_t dmg = base_dmg;
    if (is_ali) {
        dmg += calculate_ali_strength_bonus(stat_96);
    }
    const std::uint8_t stun_duration = static_cast<std::uint8_t>((element_raw & 0x0FU) + 1U);
    const std::uint16_t element_flag = static_cast<std::uint16_t>((static_cast<std::uint16_t>(element_raw & 0xF0U)) << 8U);
    const std::uint16_t total_damage = static_cast<std::uint16_t>((dmg | element_flag) + weapon_bonus_90);
    const std::uint32_t packed_a4 = (static_cast<std::uint32_t>(total_damage) << 16U) | static_cast<std::uint32_t>(stun_duration);

    return AttackDamageDesc{
        .total_damage = total_damage,
        .stun_duration = stun_duration,
        .packed_a4 = packed_a4
    };
}

/// 3D bounding box for attack hitboxes.
struct AttackBox3D {
    std::int16_t z_min{0};
    std::int16_t z_max{0};
    std::int16_t x_min{0};
    std::int16_t y_min{0};
    std::int16_t x_max{0};
    std::int16_t y_max{0};

    [[nodiscard]] constexpr AttackBox3D mirror_horizontal() const noexcept {
        return AttackBox3D{
            z_min,
            z_max,
            static_cast<std::int16_t>(-x_max),
            y_min,
            static_cast<std::int16_t>(-x_min),
            y_max
        };
    }
};

/// State of attacker entity.
struct ScannerAttackerState {
    std::uint32_t address{0};
    std::int16_t world_x{0};
    std::int16_t world_y{0};
    std::int16_t elev_z{0};
    std::int16_t ground_h{0};
    std::uint16_t facing{0};
    std::int16_t anim_frame{0};
    std::uint8_t flags_37{0};
    std::uint16_t weapon_bonus_90{0};
    std::uint16_t strength_stat_96{0};
};

/// State of candidate target entity.
struct ScannerTargetState {
    std::uint32_t address{0};
    std::int16_t type{0};
    std::int16_t world_x{0};
    std::int16_t world_y{0};
    std::int16_t elev_z{0};
    std::int16_t ground_h{0};
    std::uint8_t flags_38{0};
    std::int16_t rad_x{0};
    std::int16_t rad_y{0};
    std::int16_t height_z{0};
    std::uint8_t cooldown_82{0};
};

/// Hit dispatch callback function signature: (attacker, target, packed_a4, attack_type).
using HitCallback = std::function<void(
    ScannerAttackerState& attacker,
    ScannerTargetState& target,
    std::uint32_t packed_a4,
    std::uint16_t attack_type)>;

/// Overall result of an attack sweep.
struct AttackScanResult {
    bool suppressed_early{false};
    bool frame_found{false};
    std::size_t hit_count{0};
    bool durability_triggered{false};
    AttackDamageDesc damage_desc{};
    AttackBox3D projected_box{};
};

/// Checks if candidate target is eligible for attack scanner overlap.
bool is_scanner_target_eligible(
    const ScannerTargetState& target,
    std::uint32_t attacker_addr) noexcept;

/// Evaluates 3D AABB overlap between projected attack box and target.
bool test_scanner_aabb_overlap(
    const AttackBox3D& box,
    const ScannerTargetState& target) noexcept;

/// Executes universal attack frame sweep across a candidate target slice.
AttackScanResult scan_attack_frame_pool(
    ScannerAttackerState& attacker,
    std::uint16_t attack_type,
    std::span<const std::uint8_t> descriptor_table,
    std::span<ScannerTargetState> targets,
    const HitCallback& callback,
    std::uint32_t callback_addr = AttackScannerConfig::kCallbackAllAddr);

/// Ingress 1: scan_all_entities (0x00BC8E), sweeps 21 Group 2 slots ($FF19E8).
AttackScanResult scan_all_entities(
    ScannerAttackerState& attacker,
    std::uint16_t attack_type,
    std::span<const std::uint8_t> descriptor_table,
    std::span<ScannerTargetState> targets_21,
    const HitCallback& callback);

/// Ingress 2: scan_group3_a (0x00BCA4), sweeps 17 Group 3 slots ($FF1CD8).
AttackScanResult scan_group3_a(
    ScannerAttackerState& attacker,
    std::uint16_t attack_type,
    std::span<const std::uint8_t> descriptor_table,
    std::span<ScannerTargetState> targets_17,
    const HitCallback& callback);

/// Ingress 3: scan_group3_b (0x00BCBA), sweeps 17 Group 3 slots ($FF1CD8) with mode_flag_198b = 2.
AttackScanResult scan_group3_b(
    ScannerAttackerState& attacker,
    std::uint16_t attack_type,
    std::span<const std::uint8_t> descriptor_table,
    std::span<ScannerTargetState> targets_17,
    const HitCallback& callback,
    std::uint8_t* out_mode_flag_198b = nullptr);

/// Ingress 4: scan_single (0x00BCD8), sweeps single slot 0 ($FF19E8, Player).
AttackScanResult scan_single(
    ScannerAttackerState& attacker,
    std::uint16_t attack_type,
    std::span<const std::uint8_t> descriptor_table,
    ScannerTargetState& target_slot0,
    const HitCallback& callback);

/// Raw 64KB RAM window integration helpers.
ScannerAttackerState read_scanner_attacker_raw(
    std::span<const std::uint8_t> ram,
    std::uint32_t slot_addr);

ScannerTargetState read_scanner_target_raw(
    std::span<const std::uint8_t> ram,
    std::uint32_t slot_addr);

void write_scanner_attacker_flags_raw(
    std::span<std::uint8_t> ram,
    std::uint32_t slot_addr,
    std::uint8_t flags_37);

AttackScanResult scan_all_entities_raw(
    std::span<std::uint8_t> ram,
    std::uint32_t attacker_addr,
    std::uint16_t attack_type,
    std::span<const std::uint8_t> descriptor_table,
    const HitCallback& callback);

AttackScanResult scan_group3_a_raw(
    std::span<std::uint8_t> ram,
    std::uint32_t attacker_addr,
    std::uint16_t attack_type,
    std::span<const std::uint8_t> descriptor_table,
    const HitCallback& callback);

AttackScanResult scan_group3_b_raw(
    std::span<std::uint8_t> ram,
    std::uint32_t attacker_addr,
    std::uint16_t attack_type,
    std::span<const std::uint8_t> descriptor_table,
    const HitCallback& callback);

AttackScanResult scan_single_raw(
    std::span<std::uint8_t> ram,
    std::uint32_t attacker_addr,
    std::uint16_t attack_type,
    std::span<const std::uint8_t> descriptor_table,
    const HitCallback& callback);

} // namespace oasis::game::combat
