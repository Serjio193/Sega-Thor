#include "game/entities/entity_pool_allocators.hpp"

#include <span>

namespace oasis::game::entities {

namespace {

[[nodiscard]] std::optional<std::size_t> claim_first_free(
    std::span<std::uint16_t> slots,
    std::uint16_t entity_id) noexcept {
    for (std::size_t i = 0U; i < slots.size(); ++i) {
        if (slots[i] == 0U) {
            slots[i] = entity_id;
            return i;
        }
    }
    return std::nullopt;
}

} // namespace

std::optional<AllocatedSlot> EntityPoolAllocators::allocate(
    ObjectPoolKind pool,
    std::uint16_t entity_id) noexcept {
    if (entity_id == 0U) {
        return std::nullopt;
    }

    switch (pool) {
    case ObjectPoolKind::Effect8: {
        const auto idx = claim_first_free(effect_ids, entity_id);
        if (!idx.has_value()) {
            return std::nullopt;
        }
        return AllocatedSlot{
            pool,
            *idx,
            kEffectPool8RamBase + static_cast<std::uint32_t>(*idx) * kLightPoolStrideBytes,
            static_cast<std::uint16_t>(
                kEffectPool8VramTileBase + static_cast<std::uint16_t>(*idx) * kEffectPool8VramTileStep),
        };
    }
    case ObjectPoolKind::Special6: {
        const auto idx = claim_first_free(special_ids, entity_id);
        if (!idx.has_value()) {
            return std::nullopt;
        }
        return AllocatedSlot{
            pool,
            *idx,
            kSpecialPool6RamBase + static_cast<std::uint32_t>(*idx) * kLightPoolStrideBytes,
            0U,
        };
    }
    case ObjectPoolKind::FullEntity18: {
        const auto idx = claim_first_free(full_entity_ids, entity_id);
        if (!idx.has_value()) {
            return std::nullopt;
        }
        return AllocatedSlot{
            pool,
            *idx,
            kFullEntityPool18RamBase + static_cast<std::uint32_t>(*idx) * kFullPoolStrideBytes,
            0U,
        };
    }
    case ObjectPoolKind::EnemyEntity17: {
        const auto idx = claim_first_free(enemy_entity_ids, entity_id);
        if (!idx.has_value()) {
            return std::nullopt;
        }
        const std::uint32_t ram_addr =
            kEnemyEntityPool17RamBase + static_cast<std::uint32_t>(*idx) * kFullPoolStrideBytes;
        if (ram_addr == enemy_hud_target) {
            enemy_hud_timer = 0U;
        }
        return AllocatedSlot{pool, *idx, ram_addr, 0U};
    }
    }
    return std::nullopt;
}

void EntityPoolAllocators::free_slot(ObjectPoolKind pool, std::size_t index) noexcept {
    switch (pool) {
    case ObjectPoolKind::Effect8:
        if (index < effect_ids.size()) {
            effect_ids[index] = 0U;
        }
        break;
    case ObjectPoolKind::Special6:
        if (index < special_ids.size()) {
            special_ids[index] = 0U;
        }
        break;
    case ObjectPoolKind::FullEntity18:
        if (index < full_entity_ids.size()) {
            full_entity_ids[index] = 0U;
        }
        break;
    case ObjectPoolKind::EnemyEntity17:
        if (index < enemy_entity_ids.size()) {
            enemy_entity_ids[index] = 0U;
        }
        break;
    }
}

} // namespace oasis::game::entities
