#include "game/spirits/efreet_controller.hpp"

namespace oasis::game::spirits {

bool try_consume_spirit_ability_sp(std::uint16_t& player_sp, std::uint16_t cost) noexcept {
    if (cost == 0U) {
        return true;
    }
    // ROM 0x011B28: cmpi.w #$1, d1; blt -> requires player_sp - cost >= 1
    if (player_sp <= cost) {
        return false;
    }
    player_sp = static_cast<std::uint16_t>(player_sp - cost);
    return true;
}

void EfreetSpiritController::reset() noexcept {
    *this = EfreetSpiritController{};
}

void EfreetSpiritController::init_spawn(std::int32_t x, std::int32_t y, std::int32_t z) noexcept {
    active_ = true;
    state_ = State::Follow;
    action_ = 0x0004U;
    speed_ = 0U;
    ability_timer_ = 0U;
    sp_drain_timer_ = EfreetConfig::kSpDrainIntervalFrames;
    last_sound_cmd_ = 0U;
    helper_slot_reserved_ = false;
    world_x_ = x;
    world_y_ = y;
    world_z_ = z;
}

bool EfreetSpiritController::trigger_fire_dash(std::uint16_t& player_sp) noexcept {
    if (!active_ || !try_consume_spirit_ability_sp(player_sp, EfreetConfig::kFireDashSpCost)) {
        return false;
    }
    state_ = State::FireDash; // 0x08 (0x0126AC)
    action_ = 0x000CU;
    speed_ = EfreetConfig::kFireDashSpeed; // 0x0100
    last_sound_cmd_ = EfreetConfig::kFireDashSfx; // 0x800258
    ability_timer_ = 20U;
    return true;
}

bool EfreetSpiritController::trigger_flame_pillar(std::uint16_t& player_sp) noexcept {
    if (!active_ || !try_consume_spirit_ability_sp(player_sp, EfreetConfig::kFlamePillarSpCost)) {
        return false;
    }
    state_ = State::FlamePillar; // 0x0A (0x0127A4)
    action_ = 0x0002U;
    helper_slot_reserved_ = true; // Reserves $FF1C1C via 0x012C4E
    ability_timer_ = 24U;
    return true;
}

bool EfreetSpiritController::trigger_meltdown(std::uint16_t& player_sp, std::uint8_t spirit_level) noexcept {
    if (!active_ || !try_consume_spirit_ability_sp(player_sp, EfreetConfig::kMeltdownSpCost)) {
        return false;
    }
    state_ = State::Meltdown; // 0x0C (0x012802)
    action_ = 0x0008U;
    ability_timer_ = static_cast<std::uint16_t>(EfreetConfig::kMeltdownBaseDuration + spirit_level);
    return true;
}

void EfreetSpiritController::trigger_dismiss() noexcept {
    state_ = State::Dismiss; // 0x10 (0x012988)
    action_ = 0x000AU;
    active_ = false;
}

void EfreetSpiritController::update_frame(std::uint16_t& player_sp) noexcept {
    if (!active_) {
        return;
    }

    if (sp_drain_timer_ > 0U) {
        --sp_drain_timer_;
    }
    if (sp_drain_timer_ == 0U) {
        sp_drain_timer_ = EfreetConfig::kSpDrainIntervalFrames;
        if (player_sp > 0U) {
            --player_sp;
        }
        if (player_sp == 0U) {
            active_ = false;
            state_ = State::Dismiss;
            return;
        }
    }

    if (ability_timer_ > 0U) {
        --ability_timer_;
        if (ability_timer_ == 0U && state_ != State::Dismiss) {
            state_ = State::Follow;
            action_ = 0x0004U;
            speed_ = 0U;
            helper_slot_reserved_ = false;
        }
    }
}

} // namespace oasis::game::spirits
