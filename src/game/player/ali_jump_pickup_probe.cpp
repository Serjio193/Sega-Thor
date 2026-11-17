#include "game/player/ali_jump_pickup_probe.hpp"

#include <cstdlib>

namespace oasis::game::player {

AliJumpResult trigger_ali_jump(
    AliJumpTriggerContext& ctx,
    AliJumpEntityState& ali) noexcept {
    // ROM 0x00822C..0x008234: Check cutscene/spirit lock $FF1985
    if (ctx.cutscene_locked) {
        return AliJumpResult{.jump_type = AliJumpType::None, .triggered = false};
    }

    // ROM 0x008236..0x008244: Check d-pad movement and current sub_action == 4 (Running)
    if (ctx.dpad_held && ali.sub_action == AliJumpPickupConfig::kRunningSubAction) {
        // Running jump (0x008248..0x008274)
        ctx.running_jump_flag = 0xFFU; // st $FF197E
        ali.vel_z_16_16 = AliJumpPickupConfig::kJumpZVel16_16; // +7.0 px/frame
        ali.prev_sub_action = ali.sub_action; // backup sub_action into +0x9E
        ali.state = AliJumpPickupConfig::kRunningJumpState;     // 22 (0x16)
        ali.sub_action = AliJumpPickupConfig::kRunningJumpState;// 22 (0x16)
        ctx.input_ring_count = 0U; // clr.w $FF16EA

        return AliJumpResult{.jump_type = AliJumpType::RunningJump, .triggered = true};
    }

    // Standing jump (0x008276..0x0082AC)
    ali.vel_x_16_16 = 0;
    ali.vel_y_16_16 = 0;
    ali.vel_z_16_16 = AliJumpPickupConfig::kJumpZVel16_16; // +7.0 px/frame
    ali.prev_sub_action = ali.sub_action; // backup sub_action into +0x9E
    ali.state = AliJumpPickupConfig::kStandingJumpState;     // 16 (0x10)
    ali.sub_action = AliJumpPickupConfig::kStandingJumpState;// 16 (0x10)
    ctx.standing_jump_flag = 0U; // clr.w $FF1980
    ctx.input_ring_count = 0U;   // clr.w $FF16EA

    return AliJumpResult{.jump_type = AliJumpType::StandingJump, .triggered = true};
}

PickupProbeResult probe_pickup_entities(
    std::int32_t ali_x_16_16,
    std::int32_t ali_y_16_16,
    std::span<PickupCandidateEntity> entities) noexcept {
    for (std::size_t i = 0U; i < entities.size(); ++i) {
        auto& ent = entities[i];
        if (ent.entity_type != AliJumpPickupConfig::kPickupEntityType) {
            continue;
        }

        const auto dx_px = static_cast<std::int16_t>((ent.pos_x_16_16 - ali_x_16_16) >> 16);
        const auto dy_px = static_cast<std::int16_t>((ent.pos_y_16_16 - ali_y_16_16) >> 16);

        // ROM 0x0082AE..0x0082B6: Box [-6..+6, -6..+6]
        if (std::abs(dx_px) <= AliJumpPickupConfig::kPickupProbeRadiusPx &&
            std::abs(dy_px) <= AliJumpPickupConfig::kPickupProbeRadiusPx) {
            // Collected! Despawn entity slot (ROM 0x0082C8: clr.w (a0))
            ent.entity_type = 0U;

            // ROM 0x0082CA..0x0082E8: Construct item word and write payload
            const std::uint16_t item_word = static_cast<std::uint16_t>(
                (static_cast<std::uint16_t>(ent.item_id & 0x00FFU) << 8) |
                static_cast<std::uint16_t>(ent.quantity));

            return PickupProbeResult{
                .collected = true,
                .entity_index = i,
                .collected_item_word = item_word,
                .collected_sub_action = ent.sub_action,
                .collected_payload = ent.world_payload
            };
        }
    }

    return PickupProbeResult{
        .collected = false,
        .entity_index = 0U,
        .collected_item_word = 0U,
        .collected_sub_action = 0U,
        .collected_payload = 0
    };
}

} // namespace oasis::game::player
