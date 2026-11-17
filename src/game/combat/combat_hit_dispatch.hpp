#pragma once

#include "game/combat/weapon_hitbox_builder.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace oasis::game::combat {

struct CombatHitDispatchConfig {
    static constexpr std::uint32_t kScanPrimaryPoolAddr = 0x00BC8EU;
    static constexpr std::uint32_t kScanFxPoolAddr = 0x00BCA4U;
    static constexpr std::uint32_t kScanFxPoolMarkedAddr = 0x00BCBAU;
    static constexpr std::uint32_t kScanAliOnlyAddr = 0x00BCD8U;
    static constexpr std::uint32_t kSharedBodyAddr = 0x00BCEAU;
    static constexpr std::uint32_t kPostLoopSpecialCancelAddr = 0x00BE68U;
    static constexpr std::uint32_t kSpecialRecoilCancelAddr = 0x008530U;

    static constexpr std::uint32_t kFullEntityPoolRam = 0x00FF19E8U;
    static constexpr std::size_t kFullEntitySlotCount = 21U;
    static constexpr std::uint32_t kEnemyEntityPoolRam = 0x00FF1CD8U;
    static constexpr std::size_t kEnemyEntitySlotCount = 17U;
    static constexpr std::size_t kEnemyPoolStartIndex = 4U;
    static constexpr std::size_t kAliSlotIndex = 0U;

    static constexpr std::uint32_t kCallbackDefaultBeba = 0x0000BEBAU;
    static constexpr std::uint32_t kCallbackDefaultBe98 = 0x0000BE98U;
    static constexpr std::uint32_t kHitMarkerRam = 0x00FF198BU;
    static constexpr std::uint8_t kHitMarkerValue = 2U;
    static constexpr std::uint32_t kAttackIndexRam = 0x00FF198CU;

    static constexpr std::size_t kCombatRecordStrideBytes = 0x002CU; // 44 bytes
    static constexpr std::size_t kHitboxEntryBytes = 12U;
    static constexpr std::uint16_t kSpecialCancelIndex1 = 6U;
    static constexpr std::uint16_t kSpecialCancelIndex2 = 10U;
};

struct CombatFrameRecord {
    std::int16_t frame_id{-1};                // +0x00: frame ID (< 0 indicates table end)
    std::uint8_t base_damage{0U};              // +0x02: base damage
    std::uint8_t hit_flags{0U};                // +0x03: upper nibble = damage flags, lower nibble = hitstop-1
    std::uint32_t reserved{0U};                // +0x04: 4 bytes padding/metadata
    std::array<LocalHitbox, 3> local_boxes{};  // +0x08: Up(0), Right(1), Down(2)
};

struct AttackerEntity {
    std::size_t slot_index{0U};
    bool is_ali{false};
    std::int16_t pos_x{0};
    std::int16_t pos_y{0};
    std::int16_t pos_z{0};
    std::int16_t elevation{0};
    std::uint16_t facing{0U};    // +0x16: 0: Up, 1: Right, 2: Down, 3: Left
    std::uint8_t flags_37{0U};   // +0x37: bit 3 aborts scan / triggers special recoil cancel
    std::int16_t frame_id{0};    // +0x32
    std::int16_t power_bonus{0}; // +0x90
    std::uint16_t kill_count{0}; // +0x96: Ali kill counter
};

struct TargetEntity {
    std::size_t slot_index{0U};
    std::int16_t state{0};        // +0x00: state (> 0 active, <= 0 inactive)
    std::int16_t pos_x{0};        // +0x08: X
    std::int16_t pos_y{0};        // +0x0C: Y
    std::int16_t pos_z{0};        // +0x10: Z
    std::int16_t elevation{0};    // +0x14: elevation bias
    std::uint8_t flags_38{0U};    // +0x38: bit 3 skips weapon hit
    std::uint8_t hitstop{0U};     // +0x82: if != 0, target is already in hitstop -> skip
    std::int16_t half_width_x{0}; // +0x42: rx
    std::int16_t half_width_y{0}; // +0x44: ry
    std::int16_t height_z{0};     // +0x4A: hz
};

struct DamagePack {
    std::uint16_t damage_word{0U};
    std::uint8_t hitstop{0U};
    std::uint8_t raw_damage{0U};
    std::uint8_t hit_flags{0U};
};

struct TargetHitEvent {
    std::size_t target_slot{0U};
    std::uint32_t target_ram_addr{0U};
    DamagePack damage_pack{};
};

struct CombatHitDispatchResult {
    bool record_found{false};
    bool early_exit_flag_37{false};
    bool special_recoil_cancel{false};
    std::uint8_t hit_marker{0U};
    std::uint16_t attack_index{0U};
    std::uint32_t callback_addr{0U};
    DamagePack damage_pack{};
    WorldHitbox3D world_box{};
    std::vector<TargetHitEvent> hits{};
};

using HitCallback = std::function<void(TargetEntity& target, const AttackerEntity& attacker, const DamagePack& damage)>;

/// ROM 0x00BD36..0x00BD9E:
/// Calculates cumulative Ali damage bonus by kill count thresholds, hitstop byte, and packed damage word.
[[nodiscard]] DamagePack compute_damage_pack(
    std::uint8_t base_damage,
    std::uint8_t hit_flags,
    bool is_ali,
    std::uint16_t kill_count,
    std::int16_t power_bonus) noexcept;

/// ROM 0x00BCEA..0x00BE96:
/// Core dispatch routine searching combat records, transforming 3D hitbox, testing target containment,
/// invoking callbacks on hit, and checking post-loop special recoil cancel.
[[nodiscard]] CombatHitDispatchResult dispatch_combat_hit(
    AttackerEntity& attacker,
    std::uint16_t attack_index,
    std::span<const CombatFrameRecord> records,
    std::span<TargetEntity> targets,
    std::size_t pool_start_slot,
    std::size_t pool_slot_count,
    std::uint32_t callback_addr,
    std::uint8_t hit_marker,
    const HitCallback& callback) noexcept;

/// ROM 0x00BC8E: Callback 0xBEBA, candidate pool $FF19E8 (21 slots).
[[nodiscard]] CombatHitDispatchResult dispatch_combat_hit_primary(
    AttackerEntity& attacker,
    std::uint16_t attack_index,
    std::span<const CombatFrameRecord> records,
    std::span<TargetEntity> targets,
    const HitCallback& callback = {});

/// ROM 0x00BCA4: Callback 0xBE98, candidate pool $FF1CD8 (17 slots, slots 4..20).
[[nodiscard]] CombatHitDispatchResult dispatch_combat_hit_fx(
    AttackerEntity& attacker,
    std::uint16_t attack_index,
    std::span<const CombatFrameRecord> records,
    std::span<TargetEntity> targets,
    const HitCallback& callback = {});

/// ROM 0x00BCBA: Callback 0xBE98, sets hit_marker = 2, candidate pool $FF1CD8 (17 slots, slots 4..20).
[[nodiscard]] CombatHitDispatchResult dispatch_combat_hit_fx_marked(
    AttackerEntity& attacker,
    std::uint16_t attack_index,
    std::span<const CombatFrameRecord> records,
    std::span<TargetEntity> targets,
    const HitCallback& callback = {});

/// ROM 0x00BCD8: Callback 0xBEBA, candidate pool $FF19E8 (1 slot, Ali only).
[[nodiscard]] CombatHitDispatchResult dispatch_combat_hit_ali_only(
    AttackerEntity& attacker,
    std::uint16_t attack_index,
    std::span<const CombatFrameRecord> records,
    std::span<TargetEntity> targets,
    const HitCallback& callback = {});

} // namespace oasis::game::combat
