#include "game/spirits/shade_bow_controllers.hpp"

namespace oasis::game::spirits {

void ShadeSpiritController::reset() noexcept {
    *this = ShadeSpiritController{};
}

void ShadeSpiritController::init_spawn(std::int32_t x, std::int32_t y, std::int32_t z) noexcept {
    active_ = true;
    state_ = State::Follow;
    action_ = 0x0000U;
    player_lock_flag_ = 0U;
    ability_timer_ = 0U;
    sp_drain_timer_ = ShadeConfig::kSpDrainIntervalFrames;
    world_x_ = x;
    world_y_ = y;
    world_z_ = z;
}

bool ShadeSpiritController::cast_shadow_tether(std::uint16_t& player_sp) noexcept {
    if (!active_ || !try_consume_spirit_ability_sp(player_sp, ShadeConfig::kShadowTetherSpCost)) {
        return false;
    }
    // ROM 0x013036: State 0x06, Action 0x02, $FF1982 = 0xFF
    state_ = State::ShadowTether;
    action_ = 0x0002U;
    player_lock_flag_ = 0xFFU;
    ability_timer_ = 24U;
    return true;
}

bool ShadeSpiritController::trigger_shadow_surge() noexcept {
    if (!active_) {
        return false;
    }
    // ROM 0x013180: State 0x0C, Action 0x0E
    state_ = State::ShadowSurge;
    action_ = 0x000EU;
    player_lock_flag_ = 0U;
    ability_timer_ = 30U;
    return true;
}

void ShadeSpiritController::trigger_dismiss() noexcept {
    state_ = State::Dismiss;
    action_ = 0x0000U;
    player_lock_flag_ = 0U;
    active_ = false;
}

void ShadeSpiritController::update_frame(std::uint16_t& player_sp) noexcept {
    if (!active_) {
        return;
    }

    if (sp_drain_timer_ > 0U) {
        --sp_drain_timer_;
    }
    if (sp_drain_timer_ == 0U) {
        sp_drain_timer_ = ShadeConfig::kSpDrainIntervalFrames;
        if (player_sp > 0U) {
            --player_sp;
        }
        if (player_sp == 0U) {
            // ROM 0x0136CC: Shade despawn when SP reaches 0
            trigger_dismiss();
            return;
        }
    }

    if (ability_timer_ > 0U) {
        --ability_timer_;
        if (ability_timer_ == 0U && state_ != State::Dismiss) {
            state_ = State::Follow;
            action_ = 0x0000U;
            player_lock_flag_ = 0U;
        }
    }
}

void BowSpiritController::reset() noexcept {
    *this = BowSpiritController{};
}

void BowSpiritController::init_spawn(std::int32_t x, std::int32_t y, std::int32_t z) noexcept {
    active_ = true;
    state_ = State::Follow;
    action_ = 0x0000U;
    last_helper_pc_ = 0U;
    ability_timer_ = 0U;
    sp_drain_timer_ = BowConfig::kSpDrainIntervalFrames;
    world_x_ = x;
    world_y_ = y;
    world_z_ = z;
}

bool BowSpiritController::cast_bite(std::uint16_t& player_sp) noexcept {
    if (!active_ || !try_consume_spirit_ability_sp(player_sp, BowConfig::kBiteSpCost)) {
        return false;
    }
    // ROM 0x013934: 4 SP bite lunge
    state_ = State::Bite;
    action_ = 0x0002U;
    last_helper_pc_ = 0U;
    ability_timer_ = 16U;
    return true;
}

bool BowSpiritController::cast_pollen_vine(std::uint16_t& player_sp) noexcept {
    if (!active_ || !try_consume_spirit_ability_sp(player_sp, BowConfig::kPollenVineSpCost)) {
        return false;
    }
    // ROM 0x0139BA: 9 SP, helper 0x013FDC, State 0x04, Action 0x00
    state_ = State::PollenVine;
    action_ = 0x0000U;
    last_helper_pc_ = BowConfig::kPollenVineHelperRom;
    ability_timer_ = 24U;
    return true;
}

bool BowSpiritController::cast_plant_storm(std::uint16_t& player_sp) noexcept {
    if (!active_ || !try_consume_spirit_ability_sp(player_sp, BowConfig::kPlantStormSpCost)) {
        return false;
    }
    // ROM 0x013A18: 28 SP, State 0x0A, Action 0x10
    state_ = State::PlantStorm;
    action_ = 0x0010U;
    last_helper_pc_ = 0U;
    ability_timer_ = 48U;
    return true;
}

void BowSpiritController::trigger_dismiss() noexcept {
    state_ = State::Dismiss;
    action_ = 0x0000U;
    last_helper_pc_ = 0U;
    active_ = false;
}

void BowSpiritController::update_frame(std::uint16_t& player_sp) noexcept {
    if (!active_) {
        return;
    }

    if (sp_drain_timer_ > 0U) {
        --sp_drain_timer_;
    }
    if (sp_drain_timer_ == 0U) {
        sp_drain_timer_ = BowConfig::kSpDrainIntervalFrames;
        if (player_sp > 0U) {
            --player_sp;
        }
        if (player_sp == 0U) {
            // ROM 0x013F22: Bow despawn when SP reaches 0
            trigger_dismiss();
            return;
        }
    }

    if (ability_timer_ > 0U) {
        --ability_timer_;
        if (ability_timer_ == 0U && state_ != State::Dismiss) {
            state_ = State::Follow;
            action_ = 0x0000U;
            last_helper_pc_ = 0U;
        }
    }
}

} // namespace oasis::game::spirits
