#include "game/player/ali_input_dispatch.hpp"

#include <algorithm>

namespace oasis::game::player {

std::uint16_t poll_buffered_input(
    bool input_locked,
    std::span<const std::uint16_t> ring_words,
    std::uint16_t head_byte_offset,
    std::uint16_t ring_count,
    std::size_t max_frames) noexcept {
    if (input_locked || ring_count == 0U || ring_words.empty()) {
        return 0U;
    }

    const std::size_t frames_to_read = std::min({static_cast<std::size_t>(ring_count), max_frames, ring_words.size()});
    std::uint16_t current_byte_offset = head_byte_offset;
    std::uint16_t composite_buttons = 0U;

    for (std::size_t i = 0U; i < frames_to_read; ++i) {
        const std::size_t word_idx = (current_byte_offset & AliInputDispatchConfig::kRingOffsetMask) >> 1;
        if (word_idx < ring_words.size()) {
            composite_buttons |= ring_words[word_idx];
        }
        current_byte_offset = static_cast<std::uint16_t>((current_byte_offset + 2U) & AliInputDispatchConfig::kRingOffsetMask);
    }

    return composite_buttons;
}

AliInputDispatchResult dispatch_ali_buffered_buttons(
    AliInputDispatchContext& ctx,
    AliInputEntityState& ali,
    std::span<const std::uint16_t> ring_words,
    std::uint16_t head_byte_offset,
    std::uint16_t ring_count) noexcept {
    if (ctx.input_locked) {
        return AliInputDispatchResult{.action = AliBufferedAction::None, .buffered_buttons = 0U, .sfx_cmd = 0U};
    }

    // ROM 0x007DD2..0x007DE6: Check sub_action == 0 and flags_6f bit 7 is clear
    if (ali.sub_action == 0U && (ali.flags_6f & 0x80U) == 0U) {
        ctx.flags_1986 = 0xFFU;
    }

    if (ring_count == 0U) {
        return AliInputDispatchResult{.action = AliBufferedAction::None, .buffered_buttons = 0U, .sfx_cmd = 0U};
    }

    const std::uint16_t buffered_buttons = poll_buffered_input(
        false, ring_words, head_byte_offset, ring_count, AliInputDispatchConfig::kActionBufferFrames);

    return dispatch_ali_buffered_buttons(ctx, ali, buffered_buttons);
}

AliInputDispatchResult dispatch_ali_buffered_buttons(
    AliInputDispatchContext& ctx,
    AliInputEntityState& ali,
    std::uint16_t buffered_buttons) noexcept {
    if (ctx.input_locked) {
        return AliInputDispatchResult{.action = AliBufferedAction::None, .buffered_buttons = 0U, .sfx_cmd = 0U};
    }

    // ROM 0x007E20: Button C (bit 6: Summon / Spirit-Ball Call)
    if ((buffered_buttons & AliInputDispatchConfig::kButtonMaskC) != 0U) {
        // ROM 0x007E46: Check cutscene lock flag $FF1985
        if (ctx.flags_1985 != 0U) {
            ctx.flags_1985 = 1U;
            return AliInputDispatchResult{.action = AliBufferedAction::None, .buffered_buttons = buffered_buttons, .sfx_cmd = 0U};
        }

        // ROM 0x007E5C: Check Ali sub_action != 4 (e.g. not in un-summonable mid-action state)
        if (ali.sub_action == 4U) {
            return AliInputDispatchResult{.action = AliBufferedAction::None, .buffered_buttons = buffered_buttons, .sfx_cmd = 0U};
        }

        // ROM 0x007E66: Check active spirit ID $FF1AA4 == 0 (no spirit currently summoned)
        if (ctx.active_spirit_id != 0U) {
            return AliInputDispatchResult{.action = AliBufferedAction::None, .buffered_buttons = buffered_buttons, .sfx_cmd = 0U};
        }

        // ROM 0x007E70: Check SP > 0 at +0x8E
        if (ali.sp == 0U) {
            return AliInputDispatchResult{.action = AliBufferedAction::None, .buffered_buttons = buffered_buttons, .sfx_cmd = 0U};
        }

        // Passed all guards: summon state 20 (0x14)
        ali.state = AliInputDispatchConfig::kSummonPlayerState;
        ali.sub_action = AliInputDispatchConfig::kSummonPlayerState;
        ctx.summon_counter = static_cast<std::uint8_t>(ctx.summon_counter - 1U);

        return AliInputDispatchResult{
            .action = AliBufferedAction::SummonSpiritBall,
            .buffered_buttons = buffered_buttons,
            .sfx_cmd = AliInputDispatchConfig::kSummonSfx
        };
    }

    // ROM 0x007E28: Button A (bit 4: Attack)
    if ((buffered_buttons & AliInputDispatchConfig::kButtonMaskA) != 0U) {
        return AliInputDispatchResult{
            .action = AliBufferedAction::AttackBuffered,
            .buffered_buttons = buffered_buttons,
            .sfx_cmd = 0U
        };
    }
    if ((ctx.held_joypad & AliInputDispatchConfig::kButtonMaskA) != 0U) {
        return AliInputDispatchResult{
            .action = AliBufferedAction::AttackHeld,
            .buffered_buttons = buffered_buttons,
            .sfx_cmd = 0U
        };
    }

    // ROM 0x007E3C: Button B (bit 5: Jump)
    if ((buffered_buttons & AliInputDispatchConfig::kButtonMaskB) != 0U) {
        return AliInputDispatchResult{
            .action = AliBufferedAction::Jump,
            .buffered_buttons = buffered_buttons,
            .sfx_cmd = 0U
        };
    }

    return AliInputDispatchResult{
        .action = AliBufferedAction::None,
        .buffered_buttons = buffered_buttons,
        .sfx_cmd = 0U
    };
}

} // namespace oasis::game::player
