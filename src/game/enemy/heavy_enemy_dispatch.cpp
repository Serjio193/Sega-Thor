#include "game/enemy/heavy_enemy_dispatch.hpp"

namespace oasis::game::enemy {

std::optional<HeavyEnemyFamilySpec> lookup_heavy_enemy_family(std::uint16_t type_id) noexcept {
    for (const auto& spec : kHeavyEnemyFamilies) {
        if (spec.type_id == type_id) {
            return spec;
        }
    }
    return std::nullopt;
}

bool HeavyEnemySubstateController::init(std::uint16_t type_id, std::uint16_t param_96) noexcept {
    const auto found = lookup_heavy_enemy_family(type_id);
    if (!found.has_value()) {
        reset();
        return false;
    }
    valid_ = true;
    spec_ = *found;
    substate_ = 0U;
    timer_frames_ = spec_.init_timer_frames;
    z_ = spec_.init_z;
    state_ = spec_.init_state;
    action_ = spec_.init_action;
    param_90_ = spec_.copy_96_to_90 ? param_96 : 0U;
    return true;
}

void HeavyEnemySubstateController::reset() noexcept {
    *this = HeavyEnemySubstateController{};
}

void HeavyEnemySubstateController::set_substate(std::uint8_t raw_substate) noexcept {
    if (!valid_ || spec_.substate_count == 0U) {
        substate_ = 0U;
        return;
    }
    const std::uint8_t masked = static_cast<std::uint8_t>(raw_substate & spec_.substate_mask);
    if (masked >= spec_.substate_count) {
        substate_ = 0U;
        return;
    }
    substate_ = masked;
}

void HeavyEnemySubstateController::advance_substate() noexcept {
    if (!valid_ || spec_.substate_count == 0U) {
        substate_ = 0U;
        return;
    }
    substate_ = static_cast<std::uint8_t>((substate_ + 1U) % spec_.substate_count);
}

std::uint32_t HeavyEnemySubstateController::active_target_pc() const noexcept {
    if (!valid_) {
        return 0U;
    }
    if (substate_ == 0U && spec_.substate_0_target_pc != 0U) {
        return spec_.substate_0_target_pc;
    }
    if (substate_ == 1U && spec_.substate_1_target_pc != 0U) {
        return spec_.substate_1_target_pc;
    }
    return spec_.handler_pc;
}

} // namespace oasis::game::enemy
