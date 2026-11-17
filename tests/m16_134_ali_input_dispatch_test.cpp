#include "game/player/ali_input_dispatch.hpp"

#include <array>
#include <cassert>
#include <vector>

using namespace oasis::game::player;

static void test_rom_constants() {
    static_assert(AliInputDispatchConfig::kPollBufferedInputAddr == 0x007D8AU);
    static_assert(AliInputDispatchConfig::kDispatchAliButtonsAddr == 0x007DC8U);
    static_assert(AliInputDispatchConfig::kInputLockRam == 0x00FF1983U);
    static_assert(AliInputDispatchConfig::kInputRingRam == 0x00FF1668U);
    static_assert(AliInputDispatchConfig::kInputRingHeadRam == 0x00FF16E8U);
    static_assert(AliInputDispatchConfig::kInputRingCountRam == 0x00FF16EAU);
    static_assert(AliInputDispatchConfig::kHeldJoypadRam == 0x00FF165EU);
    static_assert(AliInputDispatchConfig::kCutsceneLockRam == 0x00FF1985U);
    static_assert(AliInputDispatchConfig::kIdleFlagRam == 0x00FF1986U);
    static_assert(AliInputDispatchConfig::kSummonCounterRam == 0x00FF1990U);
    static_assert(AliInputDispatchConfig::kActiveSpiritRam == 0x00FF1AA4U);

    static_assert(AliInputDispatchConfig::kRingBufferWordCount == 64U);
    static_assert(AliInputDispatchConfig::kRingOffsetMask == 0x007EU);
    static_assert(AliInputDispatchConfig::kActionBufferFrames == 5U);
    static_assert(AliInputDispatchConfig::kSummonPlayerState == 20U);
    static_assert(AliInputDispatchConfig::kSummonSfx == 0x00900359U);

    static_assert(AliInputDispatchConfig::kButtonMaskC == 0x0040U);
    static_assert(AliInputDispatchConfig::kButtonMaskB == 0x0020U);
    static_assert(AliInputDispatchConfig::kButtonMaskA == 0x0010U);
}

static void test_poll_buffered_input() {
    std::array<std::uint16_t, 64> ring{};
    ring.fill(0U);

    // Locked returns 0
    ring[0] = 0x0040U;
    assert(poll_buffered_input(true, ring, 0U, 5U, 5U) == 0U);

    // Empty ring count returns 0
    assert(poll_buffered_input(false, ring, 0U, 0U, 5U) == 0U);

    // Sample 3 frames without wrap: 0x01, 0x02, 0x04 -> 0x07
    ring[0] = 0x0001U;
    ring[1] = 0x0002U;
    ring[2] = 0x0004U;
    ring[3] = 0x0008U;
    assert(poll_buffered_input(false, ring, 0U, 3U, 5U) == 0x0007U);
    // If max_frames is 2, only samples 2 frames -> 0x03
    assert(poll_buffered_input(false, ring, 0U, 3U, 2U) == 0x0003U);

    // Ring wrap-around: head at offset 126 (word 63), samples 3 frames:
    // word 63 (offset 126), word 0 (offset 0), word 1 (offset 2)
    ring[63] = 0x0010U; // Attack
    ring[0]  = 0x0020U; // Jump
    ring[1]  = 0x0040U; // Summon
    assert(poll_buffered_input(false, ring, 126U, 3U, 5U) == 0x0070U);

    // Simple recent-frames helper
    std::vector<std::uint16_t> frames = {0x0010U, 0x0020U, 0x0004U};
    assert(poll_buffered_input_recent(false, frames, 2U) == 0x0030U);
    assert(poll_buffered_input_recent(true, frames, 2U) == 0x0000U);
}

static void test_dispatch_ali_buffered_priority_and_summon() {
    AliInputDispatchContext ctx{};
    AliInputEntityState ali{};
    ali.sp = 50U;
    ctx.summon_counter = 10U;

    // 1. Locked input -> None
    ctx.input_locked = true;
    auto res_locked = dispatch_ali_buffered_buttons(ctx, ali, 0x0070U);
    assert(res_locked.action == AliBufferedAction::None);
    ctx.input_locked = false;

    // 2. Button C present with SP > 0 and no active spirit -> SummonSpiritBall
    // (Priority: C > A > B even if 0x70 has C, B, A all set)
    auto res_summon = dispatch_ali_buffered_buttons(ctx, ali, 0x0070U);
    assert(res_summon.action == AliBufferedAction::SummonSpiritBall);
    assert(res_summon.sfx_cmd == AliInputDispatchConfig::kSummonSfx);
    assert(ali.state == 20U);
    assert(ali.sub_action == 20U);
    assert(ctx.summon_counter == 9U);

    // 3. Summon blocked when SP == 0 -> returns None (does not fall through to Attack)
    ali.state = 0U;
    ali.sub_action = 0U;
    ali.sp = 0U;
    ctx.held_joypad = 0x0010U; // Button A held
    auto res_c_blocked_sp = dispatch_ali_buffered_buttons(ctx, ali, 0x0040U);
    assert(res_c_blocked_sp.action == AliBufferedAction::None);

    // 4. Summon blocked when active_spirit_id != 0 -> returns None
    ali.sp = 50U;
    ctx.active_spirit_id = 1U;
    auto res_c_blocked_spirit = dispatch_ali_buffered_buttons(ctx, ali, 0x0040U);
    assert(res_c_blocked_spirit.action == AliBufferedAction::None);
    ctx.active_spirit_id = 0U;

    // 5. Summon blocked when sub_action == 4 -> returns None
    ali.sub_action = 4U;
    auto res_c_blocked_subact = dispatch_ali_buffered_buttons(ctx, ali, 0x0040U);
    assert(res_c_blocked_subact.action == AliBufferedAction::None);
    ali.sub_action = 0U;

    // 6. Summon blocked when flags_1985 != 0 -> sets flags_1985 = 1 and returns None
    ctx.flags_1985 = 0x05U;
    auto res_c_blocked_cutscene = dispatch_ali_buffered_buttons(ctx, ali, 0x0040U);
    assert(res_c_blocked_cutscene.action == AliBufferedAction::None);
    assert(ctx.flags_1985 == 1U);
    ctx.flags_1985 = 0U;
}

static void test_dispatch_attack_and_jump() {
    AliInputDispatchContext ctx{};
    AliInputEntityState ali{};
    ali.sp = 0U;

    // Button A buffered (0x10) -> AttackBuffered
    auto res_a_buf = dispatch_ali_buffered_buttons(ctx, ali, 0x0030U); // A and B both set
    assert(res_a_buf.action == AliBufferedAction::AttackBuffered);
    assert(is_attack_action(res_a_buf.action));

    // Button A held via held_joypad -> AttackHeld
    ctx.held_joypad = 0x0010U;
    auto res_a_held = dispatch_ali_buffered_buttons(ctx, ali, 0x0020U); // Only B buffered
    assert(res_a_held.action == AliBufferedAction::AttackHeld);
    assert(is_attack_action(res_a_held.action));
    ctx.held_joypad = 0U;

    // Button B buffered (0x20) -> Jump
    auto res_b_buf = dispatch_ali_buffered_buttons(ctx, ali, 0x0020U);
    assert(res_b_buf.action == AliBufferedAction::Jump);

    // No buttons -> None
    auto res_none = dispatch_ali_buffered_buttons(ctx, ali, 0x0004U);
    assert(res_none.action == AliBufferedAction::None);
}

static void test_idle_flag_and_ring_dispatch() {
    AliInputDispatchContext ctx{};
    AliInputEntityState ali{};
    ali.sub_action = 0U;
    ali.flags_6f = 0x00U;
    ctx.flags_1986 = 0U;

    std::array<std::uint16_t, 64> ring{};
    ring.fill(0U);
    ring[0] = 0x0020U; // Jump

    // sub_action == 0 and flags_6f bit 7 == 0 -> sets flags_1986 = 0xFF
    auto res = dispatch_ali_buffered_buttons(ctx, ali, ring, 0U, 1U);
    assert(ctx.flags_1986 == 0xFFU);
    assert(res.action == AliBufferedAction::Jump);

    // If sub_action != 0, flags_1986 is not set
    ctx.flags_1986 = 0U;
    ali.sub_action = 1U;
    [[maybe_unused]] auto res2 = dispatch_ali_buffered_buttons(ctx, ali, ring, 0U, 1U);
    assert(ctx.flags_1986 == 0U);

    // If flags_6f bit 7 is set, flags_1986 is not set
    ali.sub_action = 0U;
    ali.flags_6f = 0x80U;
    [[maybe_unused]] auto res3 = dispatch_ali_buffered_buttons(ctx, ali, ring, 0U, 1U);
    assert(ctx.flags_1986 == 0U);
}

int main() {
    test_rom_constants();
    test_poll_buffered_input();
    test_dispatch_ali_buffered_priority_and_summon();
    test_dispatch_attack_and_jump();
    test_idle_flag_and_ring_dispatch();
    return 0;
}
