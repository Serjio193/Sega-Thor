#pragma once

#include <cstdint>

namespace oasis::game::ui {

inline constexpr std::uint32_t kVintTailEntryAddr = 0x002380U;
inline constexpr std::uint32_t kSoundDriverTickAddr = 0x060000U;
inline constexpr std::uint32_t kHudSlideDirRam = 0xFF1859U;
inline constexpr std::uint32_t kHudSlideOffsetRam = 0xFF185AU;
inline constexpr std::int16_t kHudSlideFwdStop = 0x20;   // 32
inline constexpr std::int16_t kHudSlideRevStop = 0x138;  // 312
inline constexpr std::int16_t kHudSlideWrapLimit = 0x158;// 344
inline constexpr std::int16_t kHudSlideStepPixels = 2;

/// Models 0x002380..0x0023CA HUD slide offset ($FF185A) step by 2 px:
/// - reverse_dir == false ($FF1859 == 0): stops at 0x20, else +2 wrapping >= 0x158 -> 0.
/// - reverse_dir == true  ($FF1859 != 0): stops at 0x138, else -2 wrapping < 0 -> 0x158.
[[nodiscard]] std::int16_t step_hud_slide_offset(
    std::int16_t offset,
    bool reverse_dir) noexcept;

struct VintTailState {
    std::uint64_t sound_ticks{0};
    std::int16_t hud_slide_offset{kHudSlideFwdStop};
    bool hud_slide_reverse{false};
};

/// Executes the V-Int tail: ticks the sound driver (0x060000) and steps HUD slide.
void step_vint_tail(VintTailState& state) noexcept;

} // namespace oasis::game::ui
