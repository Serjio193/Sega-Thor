#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::game::player {

struct AliInputDispatchConfig {
    static constexpr std::uint32_t kPollBufferedInputAddr = 0x007D8AU;
    static constexpr std::uint32_t kDispatchAliButtonsAddr = 0x007DC8U;

    static constexpr std::uint32_t kInputLockRam = 0x00FF1983U;
    static constexpr std::uint32_t kInputRingRam = 0x00FF1668U;
    static constexpr std::uint32_t kInputRingHeadRam = 0x00FF16E8U;
    static constexpr std::uint32_t kInputRingCountRam = 0x00FF16EAU;
    static constexpr std::uint32_t kHeldJoypadRam = 0x00FF165EU;
    static constexpr std::uint32_t kCutsceneLockRam = 0x00FF1985U;
    static constexpr std::uint32_t kIdleFlagRam = 0x00FF1986U;
    static constexpr std::uint32_t kSummonCounterRam = 0x00FF1990U;
    static constexpr std::uint32_t kActiveSpiritRam = 0x00FF1AA4U;

    static constexpr std::size_t kRingBufferWordCount = 64U;
    static constexpr std::size_t kRingBufferByteSize = 128U;
    static constexpr std::uint16_t kRingOffsetMask = 0x007EU;

    static constexpr std::size_t kActionBufferFrames = 5U;
    static constexpr std::uint16_t kSummonPlayerState = 20U; // 0x14
    static constexpr std::uint32_t kSummonSfx = 0x00900359U;

    static constexpr std::uint16_t kButtonMaskC = 0x0040U; // bit 6: Summon / Call
    static constexpr std::uint16_t kButtonMaskB = 0x0020U; // bit 5: Jump
    static constexpr std::uint16_t kButtonMaskA = 0x0010U; // bit 4: Attack
};

enum class AliBufferedAction : std::uint8_t {
    None = 0,
    SummonSpiritBall,
    AttackBuffered,
    AttackHeld,
    Jump,
};

[[nodiscard]] constexpr bool is_attack_action(AliBufferedAction action) noexcept {
    return action == AliBufferedAction::AttackBuffered || action == AliBufferedAction::AttackHeld;
}

struct AliInputDispatchContext {
    bool input_locked{false};          // $FF1983 != 0
    std::uint8_t flags_1985{0U};       // $FF1985
    std::uint8_t flags_1986{0U};       // $FF1986
    std::uint8_t summon_counter{0U};   // $FF1990
    std::uint16_t active_spirit_id{0U};// $FF1AA4
    std::uint16_t held_joypad{0U};      // $FF165E
};

struct AliInputEntityState {
    std::uint16_t sub_action{0U};      // Ali +0x04
    std::uint16_t state{0U};           // Ali +0x2A
    std::uint8_t flags_6f{0U};         // Ali +0x6F
    std::uint16_t sp{0U};              // Ali +0x8E
};

struct AliInputDispatchResult {
    AliBufferedAction action{AliBufferedAction::None};
    std::uint16_t buffered_buttons{0U};
    std::uint32_t sfx_cmd{0U};
};

/// ROM 0x007D8A..0x007DC6:
/// Polls and bitwise-ORs up to max_frames words from the 64-frame input ring buffer ($FF1668).
/// Returns 0 if input_locked ($FF1983 != 0) or ring_count == 0.
[[nodiscard]] std::uint16_t poll_buffered_input(
    bool input_locked,
    std::span<const std::uint16_t> ring_words,
    std::uint16_t head_byte_offset,
    std::uint16_t ring_count,
    std::size_t max_frames) noexcept;

/// Simple overload for recent frames slice (head-to-tail).
[[nodiscard]] constexpr std::uint16_t poll_buffered_input_recent(
    bool input_locked,
    std::span<const std::uint16_t> recent_frames,
    std::size_t max_frames = AliInputDispatchConfig::kActionBufferFrames) noexcept {
    if (input_locked) {
        return 0U;
    }
    const std::size_t count = recent_frames.size() < max_frames ? recent_frames.size() : max_frames;
    std::uint16_t result = 0U;
    for (std::size_t i = 0U; i < count; ++i) {
        result |= recent_frames[i];
    }
    return result;
}

/// ROM 0x007DC8..0x007E9C:
/// Evaluates buffered composite inputs across up to 5 frames.
/// Priority: Button C (bit 6: summon) -> Button A (bit 4: attack) -> Button B (bit 5: jump).
[[nodiscard]] AliInputDispatchResult dispatch_ali_buffered_buttons(
    AliInputDispatchContext& ctx,
    AliInputEntityState& ali,
    std::span<const std::uint16_t> ring_words,
    std::uint16_t head_byte_offset,
    std::uint16_t ring_count) noexcept;

/// Overload taking pre-polled buffered buttons.
[[nodiscard]] AliInputDispatchResult dispatch_ali_buffered_buttons(
    AliInputDispatchContext& ctx,
    AliInputEntityState& ali,
    std::uint16_t buffered_buttons) noexcept;

} // namespace oasis::game::player
