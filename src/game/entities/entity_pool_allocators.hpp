#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace oasis::game::entities {

inline constexpr std::uint32_t kEffectPool8Addr = 0x00D9BEU;
inline constexpr std::uint32_t kSpecialPool6Addr = 0x00D9F0U;
inline constexpr std::uint32_t kFullEntityPool18Addr = 0x00DA1AU;
inline constexpr std::uint32_t kEnemyEntityPool17Addr = 0x00DA2AU;

inline constexpr std::uint32_t kEffectPool8RamBase = 0xFF2ABCU;
inline constexpr std::uint32_t kSpecialPool6RamBase = 0xFF2D8CU;
inline constexpr std::uint32_t kFullEntityPool18RamBase = 0xFF1C1CU;
inline constexpr std::uint32_t kEnemyEntityPool17RamBase = 0xFF1CD8U;

inline constexpr std::size_t kEffectPool8Capacity = 8U;
inline constexpr std::size_t kSpecialPool6Capacity = 6U;
inline constexpr std::size_t kFullEntityPool18Capacity = 18U;
inline constexpr std::size_t kEnemyEntityPool17Capacity = 17U;

inline constexpr std::uint32_t kLightPoolStrideBytes = 0x5AU; // 90
inline constexpr std::uint32_t kFullPoolStrideBytes = 0xBCU;  // 188
inline constexpr std::uint16_t kEffectPool8VramTileBase = 0x0694U;
inline constexpr std::uint16_t kEffectPool8VramTileStep = 9U;

enum class ObjectPoolKind : std::uint8_t {
    Effect8,
    Special6,
    FullEntity18,
    EnemyEntity17,
};

struct AllocatedSlot {
    ObjectPoolKind pool{ObjectPoolKind::Effect8};
    std::size_t index{0};
    std::uint32_t ram_addr{0};
    std::uint16_t vram_tile{0};
};

/// Models the 4 fixed-stride WRAM entity/effect pool allocators (0x00D9BE..0x00DA60).
class EntityPoolAllocators {
public:
    std::array<std::uint16_t, kEffectPool8Capacity> effect_ids{};
    std::array<std::uint16_t, kSpecialPool6Capacity> special_ids{};
    std::array<std::uint16_t, kFullEntityPool18Capacity> full_entity_ids{};
    std::array<std::uint16_t, kEnemyEntityPool17Capacity> enemy_entity_ids{};

    std::uint8_t enemy_hud_timer{0};    // $FF184F
    std::uint32_t enemy_hud_target{0};  // $FF1850 (slot RAM addr)

    /// Scans the requested pool for the first slot where entity_id == 0,
    /// claims it with `entity_id`, and returns AllocatedSlot (or nullopt if full
    /// or entity_id == 0). For EnemyEntity17, clears enemy_hud_timer if the
    /// reused slot's ram_addr equals enemy_hud_target.
    [[nodiscard]] std::optional<AllocatedSlot> allocate(
        ObjectPoolKind pool,
        std::uint16_t entity_id) noexcept;

    void free_slot(ObjectPoolKind pool, std::size_t index) noexcept;
};

} // namespace oasis::game::entities
