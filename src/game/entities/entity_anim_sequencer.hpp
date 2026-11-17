#pragma once

#include <cstdint>
#include <span>

namespace oasis::game::entities {

inline constexpr std::uint32_t kEntityAnimSequencerAddr = 0x008FF0U;
inline constexpr std::uint32_t kEntityPhysicsStepperAddr = 0x009042U;
inline constexpr std::uint16_t kAnimKeyframeLastMask = 0x8000U;
inline constexpr std::uint8_t kAnimFlagAnimCompleted = 0x02U; // $37 bit 1
inline constexpr std::uint8_t kAnimFlagLastFrame = 0x04U;     // $37 bit 2
inline constexpr std::uint8_t kAnimFlagSwingHitOnce = 0x08U;  // $37 bit 3

/// 4-byte entity animation keyframe (frame_id, flags_dur).
struct AnimKeyframe4 {
    std::uint16_t frame_id{0};
    std::uint16_t flags_dur{0};

    [[nodiscard]] constexpr bool is_last_frame() const noexcept {
        return (flags_dur & kAnimKeyframeLastMask) != 0U;
    }
};

/// Entity animation sequencer state (+0x2E, +0x30, +0x32, +0x37).
struct EntityAnimState {
    std::uint16_t cursor_bytes{0};   // +0x30 (byte offset, stride 4)
    std::uint16_t timer_2e{0};       // +0x2E
    std::uint16_t frame_id_32{0};    // +0x32
    std::uint8_t combat_flags_37{0}; // +0x37 (bit1=completed, bit2=last, bit3=swing_hit_once)
};

/// Models 0x008FF0..0x009041: clears bit 3 (swing_hit_once), wraps cursor if
/// bit 2 (last_frame) was set, loads keyframe at cursor_bytes / 4, computes
/// timer_2e = max(0, ((flags_dur & 0x7FFF) >> 1) - 1), and sets bit 1
/// (anim_completed) when timer_2e == 0 && last_frame.
bool step_anim_keyframe(
    std::span<const AnimKeyframe4> keyframes,
    EntityAnimState& state) noexcept;

/// Advances animation by 1 tick: decrements timer_2e if > 0 (setting
/// anim_completed when timer_2e reaches 0 on last_frame), or calls
/// step_anim_keyframe() when timer_2e == 0. Returns true if a new keyframe
/// was loaded.
bool step_anim_tick(
    std::span<const AnimKeyframe4> keyframes,
    EntityAnimState& state) noexcept;

} // namespace oasis::game::entities
