#include "game/spirits/dytto_controller.hpp"

#include <algorithm>
#include <cmath>

namespace oasis::game::spirits {

namespace {
constexpr double kPi = 3.14159265358979323846;
} // namespace

void DyttoSpiritController::reset() noexcept {
    *this = DyttoSpiritController{};
}

void DyttoSpiritController::init_spawn(std::int32_t spawn_x,
                                       std::int32_t spawn_y,
                                       std::int32_t spawn_z,
                                       std::uint16_t facing,
                                       bool altar_awakening) noexcept {
    active_ = true;
    type_id_ = DyttoEntityConfig::kEntityType;
    collision_flags_ = DyttoEntityConfig::kCollisionFlags;
    vram_pattern_ = DyttoEntityConfig::kInitialVramPattern;
    vram_tile_base_ = DyttoEntityConfig::kVramTileBase;
    rom_gfx_source_ = DyttoEntityConfig::kRomGfxSource;

    max_sp_ = DyttoEntityConfig::kMaxSp;
    current_sp_ = DyttoEntityConfig::kMaxSp;
    sp_drain_rate_ = DyttoEntityConfig::kSpDrainRate;
    sp_drain_timer_ = DyttoEntityConfig::kSpDrainRate;

    world_x_ = spawn_x;
    world_y_ = spawn_y;
    world_z_ = spawn_z + (altar_awakening ? DyttoEntityConfig::kElevationDeltaPx : 0);
    x_fp_ = spawn_x * 0x10000;
    y_fp_ = spawn_y * 0x10000;
    facing_ = facing;

    state_ = altar_awakening ? State::AppearCutscene : State::Default;
    action_ = altar_awakening ? DyttoEntityConfig::kAppearAction : DyttoEntityConfig::kDefaultAction;
    appear_timer_ = 0U;
    events_ = DyttoAwakeningEvents{};

    current_angle_ = 0U;
    velocity_fp_ = 0;
    target_offset_x_ = 0;
    target_offset_y_ = -16;
    dma_requested_ = true;
}

std::uint8_t DyttoSpiritController::binary_atan2(std::int32_t dx, std::int32_t dy) noexcept {
    if (dx == 0 && dy == 0) {
        return 0U;
    }
    const double rad = std::atan2(static_cast<double>(dy), static_cast<double>(dx));
    double scaled = rad * (256.0 / (2.0 * kPi));
    if (scaled < 0.0) {
        scaled += 256.0;
    }
    const int quantized = static_cast<int>(std::lround(scaled)) & 0xFF;
    return static_cast<std::uint8_t>(quantized);
}

std::uint8_t DyttoSpiritController::step_angle_toward(
    std::uint8_t current,
    std::uint8_t target,
    std::uint8_t step) noexcept {
    int diff = static_cast<int>(target) - static_cast<int>(current);
    if (diff > 127) {
        diff -= 256;
    } else if (diff < -128) {
        diff += 256;
    }
    const int s = static_cast<int>(step);
    if (std::abs(diff) <= s) {
        return target;
    }
    const int next = (static_cast<int>(current) + (diff > 0 ? s : -s)) & 0xFF;
    return static_cast<std::uint8_t>(next);
}

std::int32_t DyttoSpiritController::damp_velocity_asr7(std::int32_t vel_fp) noexcept {
    return vel_fp >> DyttoEntityConfig::kVelocityDampingShift;
}

void DyttoSpiritController::dispatch_autonomous_command(
    std::uint16_t prng_word,
    bool cutscene_locked,
    std::int32_t player_x,
    std::int32_t player_y) noexcept {
    // ROM 0x014932 (State 0x00): if cutscene lock $FF1987 != 0 or PRNG > 0x9000, wander around Ali.
    if (!cutscene_locked) {
        if (prng_word <= DyttoAbilityConfig::kPrngMagicBurstMax) {
            // 0x0149E0 (12.5%): Magic Burst -> State 0x06, Action 0x02, SFX 0x800251
            state_ = State::MagicAttack1;
            action_ = 0x0002U;
            last_sound_cmd_ = DyttoAbilityConfig::kSfxAbilityStart;
            return;
        }
        if (prng_word <= DyttoAbilityConfig::kPrngSpreadMax) {
            // 0x014A22 (25.0%): Dytto 3-Way Spread -> State 0x10, Action 0x12
            state_ = State::SpecialSpread;
            action_ = 0x0012U;
            return;
        }
        if (prng_word <= DyttoAbilityConfig::kPrngContactBuffMax) {
            // 0x014A34 (18.75%): Return-to-Ali Contact Buff -> State 0x16, Action 0x18, SFX 0x800251
            state_ = State::ReturnToAli;
            action_ = 0x0018U;
            velocity_fp_ = 0x00040000;
            wander_timer_ = 0x0018U;
            last_sound_cmd_ = DyttoAbilityConfig::kSfxAbilityStart;
            return;
        }
    }

    // 0x014958 (43.75% or locked): Wander target halfway between (256, 320) and Ali + PRNG jitter (ASR.W #3)
    const std::int32_t jitter_x = static_cast<std::int16_t>(prng_word) >> 3;
    const std::int32_t mid_x = ((256 + player_x) / 2) + jitter_x;
    const std::int32_t mid_y = (320 + player_y) / 2;
    target_offset_x_ = static_cast<std::int16_t>(mid_x - player_x);
    target_offset_y_ = static_cast<std::int16_t>(mid_y - player_y);
    wander_timer_ = 10U;
    state_ = State::FollowPlayer;
    action_ = DyttoEntityConfig::kDefaultAction;
}

void DyttoSpiritController::trigger_knockback(
    std::int32_t vx_fp,
    std::int32_t vy_fp,
    std::int32_t vz_fp) noexcept {
    // ROM 0x014EE6 (State 0x0E): enter knockback deceleration
    kb_vx_fp_ = vx_fp;
    kb_vy_fp_ = vy_fp;
    kb_vz_fp_ = vz_fp;
    state_ = State::KnockbackRecovery;
}

void DyttoSpiritController::update_frame(
    std::int32_t player_x,
    std::int32_t player_y,
    std::span<NativeEntity> entities,
    std::uint16_t prng_word,
    bool cutscene_locked) noexcept {
    if (!active_) {
        return;
    }

    // Step active 0x0024 projectiles (State 0x0C / 0x12 / 0x14) and apply Water damage (0x014E86)
    if (!projectiles_.empty()) {
        const std::uint32_t hit_sfx = step_dytto_projectiles(projectiles_, entities);
        if (hit_sfx != 0U) {
            last_sound_cmd_ = hit_sfx;
        }
    }

    if (state_ == State::AppearCutscene) {
        ++appear_timer_;
        if (appear_timer_ >= DyttoEntityConfig::kTimelineBurstSfxFrame) events_.burst_sfx_triggered = true;
        if (appear_timer_ >= DyttoEntityConfig::kTimelinePalettePulseFrame) events_.palette_pulse_triggered = true;
        if (appear_timer_ >= DyttoEntityConfig::kTimelineDialogueCompleteFrame) events_.dialogue_complete_handoff = true;
        if (appear_timer_ >= DyttoEntityConfig::kTimelineCameraUnlockFrame) events_.camera_unlocked = true;
        if (appear_timer_ >= DyttoEntityConfig::kTimelineCompleteFrame) {
            events_.cutscene_completed = true;
            state_ = State::FollowPlayer;
            action_ = DyttoEntityConfig::kDefaultAction;
        }
        return;
    }

    if (state_ == State::IdleDispatch) {
        dispatch_autonomous_command(prng_word, cutscene_locked, player_x, player_y);
        return;
    }

    if (state_ == State::MagicAttack1) {
        // State 0x06 (0x014ABC) -> State 0x08 (MagicAttack2)
        state_ = State::MagicAttack2;
        magic_frame_counter_ = 0U;
        return;
    }

    if (state_ == State::MagicAttack2) {
        // State 0x08 (0x014AEA): spawns 0x0024 via 0x015136 on every 3rd frame
        ++magic_frame_counter_;
        if ((magic_frame_counter_ % 3U) == 0U) {
            spawn_dytto_magic_burst_projectile(projectiles_, world_x_, world_y_, world_z_, current_angle_);
            last_sound_cmd_ = DyttoAbilityConfig::kSfxProjectileSpawn;
            state_ = State::MagicRecovery;
        }
        return;
    }

    if (state_ == State::MagicRecovery) {
        // State 0x0A (0x014B50) -> wander State 0x04
        wander_timer_ = 10U;
        state_ = State::FollowPlayer;
        action_ = DyttoEntityConfig::kDefaultAction;
        return;
    }

    if (state_ == State::SpecialSpread) {
        // State 0x10 (0x014A72 -> 0x015214): guarded on $30(A6) == 0x0018 (Dytto)
        if (type_id_ == DyttoEntityConfig::kEntityType) {
            spawn_dytto_spread_projectiles(projectiles_, world_x_, world_y_, world_z_, current_angle_);
            last_sound_cmd_ = DyttoAbilityConfig::kSfxProjectileSpawn;
        }
        state_ = State::FollowPlayer;
        action_ = DyttoEntityConfig::kDefaultAction;
        return;
    }

    if (state_ == State::ReturnToAli) {
        // State 0x16 (0x014B7C): approach Ali until overlap -> State 0x18 (AliContactBuff)
        const std::int32_t dx = player_x - world_x_;
        const std::int32_t dy = player_y - world_y_;
        if (std::abs(dx) <= 16 && std::abs(dy) <= 16) {
            state_ = State::AliContactBuff;
            return;
        }
        current_angle_ = step_angle_toward(current_angle_, binary_atan2(dx, dy), DyttoEntityConfig::kAngularTurnStep);
        const double rad = static_cast<double>(current_angle_) * ((2.0 * kPi) / 256.0);
        x_fp_ += static_cast<std::int32_t>(std::llround(std::cos(rad) * 0x00020000));
        y_fp_ += static_cast<std::int32_t>(std::llround(std::sin(rad) * 0x00020000));
        world_x_ = x_fp_ >> 16;
        world_y_ = y_fp_ >> 16;
        return;
    }

    if (state_ == State::AliContactBuff) {
        // State 0x18 (0x014BB8): invoke callback $FF1940 = 0x014C04 / jsr 0x00BCE2
        ali_buff_triggered_ = true;
        last_callback_addr_ = DyttoAbilityConfig::kAliBuffCallbackAddr;
        state_ = State::FollowPlayer;
        action_ = DyttoEntityConfig::kDefaultAction;
        return;
    }

    if (state_ == State::KnockbackRecovery) {
        // State 0x0E (0x014EE6): vx/vy -= 0x8000, vz -= 0xA000
        x_fp_ += kb_vx_fp_;
        y_fp_ += kb_vy_fp_;
        world_x_ = x_fp_ >> 16;
        world_y_ = y_fp_ >> 16;
        world_z_ += (kb_vz_fp_ >> 16);
        kb_vx_fp_ = decelerate_toward_zero(kb_vx_fp_, DyttoAbilityConfig::kKnockbackXyDampFp);
        kb_vy_fp_ = decelerate_toward_zero(kb_vy_fp_, DyttoAbilityConfig::kKnockbackXyDampFp);
        kb_vz_fp_ = decelerate_toward_zero(kb_vz_fp_, DyttoAbilityConfig::kKnockbackZDampFp);
        if (kb_vx_fp_ == 0 && kb_vy_fp_ == 0 && kb_vz_fp_ == 0) {
            state_ = State::FollowPlayer;
            action_ = DyttoEntityConfig::kDefaultAction;
        }
        return;
    }

    if (state_ == State::FollowPlayer) {
        if (wander_timer_ > 0U) {
            --wander_timer_;
            if (wander_timer_ == 0U) {
                state_ = State::IdleDispatch;
                return;
            }
        }

        const std::int32_t tx = player_x + static_cast<std::int32_t>(target_offset_x_);
        const std::int32_t ty = player_y + static_cast<std::int32_t>(target_offset_y_);
        const std::int32_t dx = tx - world_x_;
        const std::int32_t dy = ty - world_y_;

        if (dx == 0 && dy == 0) {
            velocity_fp_ = 0;
            return;
        }

        const std::uint8_t target_angle = binary_atan2(dx, dy);
        current_angle_ = step_angle_toward(current_angle_, target_angle, DyttoEntityConfig::kAngularTurnStep);

        const std::int32_t dist_sq = dx * dx + dy * dy;
        const std::int32_t dist = static_cast<std::int32_t>(std::sqrt(static_cast<double>(dist_sq)));
        const std::int32_t desired_vel_fp = std::min<std::int32_t>(dist * 0x40000, 0x2000000);
        velocity_fp_ = desired_vel_fp;
        const std::int32_t step_fp = std::max<std::int32_t>(damp_velocity_asr7(velocity_fp_), 0x10000);

        const double rad = static_cast<double>(current_angle_) * ((2.0 * kPi) / 256.0);
        const std::int32_t vx_fp = static_cast<std::int32_t>(std::llround(std::cos(rad) * static_cast<double>(step_fp)));
        const std::int32_t vy_fp = static_cast<std::int32_t>(std::llround(std::sin(rad) * static_cast<double>(step_fp)));

        if (std::abs(dx) <= 1 && std::abs(dy) <= 1) {
            world_x_ = tx;
            world_y_ = ty;
            x_fp_ = tx * 0x10000;
            y_fp_ = ty * 0x10000;
        } else {
            x_fp_ += vx_fp;
            y_fp_ += vy_fp;
            world_x_ = x_fp_ >> 16;
            world_y_ = y_fp_ >> 16;
        }
    }
}

} // namespace oasis::game::spirits
