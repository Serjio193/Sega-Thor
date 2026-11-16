#include "game/enemy/dungeon_enemy_dispatch.hpp"

namespace oasis::game::enemy {

std::optional<DungeonEnemyFamilySpec> lookup_dungeon_enemy_family(std::uint16_t type_id) noexcept {
    for (const auto& spec : kDungeonEnemyFamilies) {
        if (spec.type_id == type_id) {
            return spec;
        }
    }
    return std::nullopt;
}

std::uint16_t lookup_006c_variant_param(std::uint8_t variant_index) noexcept {
    if (variant_index >= kDungeonEnemy006CVariants.size()) {
        return 0U;
    }
    return kDungeonEnemy006CVariants[variant_index];
}

std::uint32_t resolve_0072_substate_target(std::uint8_t substate) noexcept {
    if (substate >= kDungeonEnemy0072Substates.size()) {
        return 0U;
    }
    return kDungeonEnemy0072Substates[substate];
}

bool DungeonEnemySubstateController::init(std::uint16_t type_id, std::uint8_t variant_006c) noexcept {
    const auto found = lookup_dungeon_enemy_family(type_id);
    if (!found.has_value()) {
        reset();
        return false;
    }
    valid_ = true;
    spec_ = *found;
    substate_ = 0U;
    variant_index_ = 0U;
    param_90_ = 0U;
    if (type_id == 0x006CU) {
        set_variant_006c(variant_006c);
    }
    return true;
}

void DungeonEnemySubstateController::reset() noexcept {
    *this = DungeonEnemySubstateController{};
}

void DungeonEnemySubstateController::set_substate(std::uint8_t substate) noexcept {
    if (!valid_ || spec_.substate_count == 0U || substate >= spec_.substate_count) {
        substate_ = 0U;
        return;
    }
    substate_ = substate;
}

void DungeonEnemySubstateController::advance_substate() noexcept {
    if (!valid_ || spec_.substate_count == 0U) {
        substate_ = 0U;
        return;
    }
    substate_ = static_cast<std::uint8_t>((substate_ + 1U) % spec_.substate_count);
}

void DungeonEnemySubstateController::set_variant_006c(std::uint8_t variant_index) noexcept {
    if (!valid_ || spec_.type_id != 0x006CU) {
        return;
    }
    const std::uint8_t clamped = static_cast<std::uint8_t>(variant_index & 0x03U);
    variant_index_ = clamped;
    param_90_ = lookup_006c_variant_param(clamped);
}

std::uint32_t DungeonEnemySubstateController::active_target_pc() const noexcept {
    if (!valid_) {
        return 0U;
    }
    if (spec_.type_id == 0x0072U) {
        return resolve_0072_substate_target(substate_);
    }
    return spec_.handler_pc;
}

} // namespace oasis::game::enemy
