#pragma once

#include <cstdint>

namespace oasis::game::ui {

inline constexpr std::uint32_t kTryOpenRingMenuAddr = 0x004416U;
inline constexpr std::uint32_t kRingMenuInputPollAddr = 0x004480U;

inline constexpr std::uint32_t kMenuBlockRamAddr = 0x00FF1A6EU;
inline constexpr std::uint32_t kPlayerStateRamAddr = 0x00FF19ECU;
inline constexpr std::uint32_t kRingCursorRamAddr = 0x00FF0E50U;
inline constexpr std::uint32_t kButtonSwapRamAddr = 0x00FF1996U;
inline constexpr std::uint32_t kJoypadRamAddr = 0x00FF165EU;

inline constexpr std::uint8_t kMaxOpenPlayerState = 2U;
inline constexpr std::uint32_t kRingMenuErrorSfxCmd = 0x00A00253U;
inline constexpr std::uint32_t kRingMenuCloseSfxCmd = 0x00A00347U;

inline constexpr std::uint8_t kJoypadUp = 0x01U;
inline constexpr std::uint8_t kJoypadDown = 0x02U;
inline constexpr std::uint8_t kJoypadLeft = 0x04U;
inline constexpr std::uint8_t kJoypadRight = 0x08U;
inline constexpr std::uint8_t kJoypadB = 0x10U;
inline constexpr std::uint8_t kJoypadC = 0x20U;
inline constexpr std::uint8_t kJoypadA = 0x40U;
inline constexpr std::uint8_t kJoypadStart = 0x80U;

enum class RingMenuAction : std::uint8_t {
    None = 0,
    Close,
    Confirm,
    Start,
    Up,
    Down,
    Left,
    Right,
};

/// Models 0x004416..0x004438: returns true when $FF1A6E == 0 and $FF19EC <= 2;
/// otherwise callers emit kRingMenuErrorSfxCmd (0x00A00253).
[[nodiscard]] bool can_open_ring_menu(
    std::uint8_t menu_block,
    std::uint8_t player_state) noexcept;

/// Models 0x004480..0x00455C: decodes $FF165E joypad byte with $FF1996 A/BC swap.
/// Normal: A closes (0x00A00347), B|C confirms; Swapped: B|C closes, A confirms.
[[nodiscard]] RingMenuAction decode_ring_menu_action(
    std::uint8_t joypad,
    bool button_swap) noexcept;

} // namespace oasis::game::ui
