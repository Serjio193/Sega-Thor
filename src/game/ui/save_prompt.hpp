#pragma once

#include <cstdint>

namespace oasis::game::ui {

inline constexpr std::uint32_t kSavePromptEntryAddr = 0x005472U;
inline constexpr std::uint32_t kSavePromptGuardAddr = 0x005488U;
inline constexpr std::uint32_t kSavePromptLoopAddr = 0x0054E2U;
inline constexpr std::uint32_t kSramSaveRoutineAddr = 0x00001726U;

inline constexpr std::uint8_t kSaveChoiceYes = 3U;
inline constexpr std::uint8_t kSaveChoiceNo = 4U;
inline constexpr std::uint8_t kSaveDefaultChoice = kSaveChoiceNo;
inline constexpr std::uint8_t kSaveMaxPlayerState = 2U;
inline constexpr std::uint8_t kRoomSaveAllowedBit = 6U;

inline constexpr std::uint32_t kSavePromptToggleSfx = 0x006003A5U;
inline constexpr std::uint32_t kSavePromptConfirmSfx = 0x00A003AAU;
inline constexpr std::uint32_t kSavePromptCancelSfx = 0x00A00347U;
inline constexpr std::uint32_t kSavePromptErrorSfx = 0x00A00253U;

inline constexpr std::uint8_t kSavePromptBtnUp = 0x01U;         // bit 0
inline constexpr std::uint8_t kSavePromptBtnDown = 0x02U;       // bit 1
inline constexpr std::uint8_t kSavePromptBtnA = 0x10U;          // bit 4
inline constexpr std::uint8_t kSavePromptBtnCancelMask = 0x60U; // bits 5..6 (B/C)

enum class SavePromptOutcome : std::uint8_t {
    Pending = 0,
    ConfirmedSave,
    Cancelled,
};

struct SavePromptStepResult {
    std::uint8_t next_choice{kSaveDefaultChoice};
    SavePromptOutcome outcome{SavePromptOutcome::Pending};
    std::uint32_t sound_cmd{0U};
};

/// Models 0x005488..0x0054C6: requires $FF1996 == 0, $FF17C1 == 0,
/// $FF19EC <= 2, and (room_save_allowed || debug_override); else 0x00A00253.
[[nodiscard]] bool can_open_save_prompt(
    std::uint8_t button_swap_ff1996,
    std::uint8_t flag_ff17c1,
    std::uint8_t player_state,
    bool room_save_allowed,
    bool debug_override) noexcept;

/// Models 0x0054E2..0x00555E: steps YES (3) / NO (4) save prompt state machine.
[[nodiscard]] SavePromptStepResult step_save_prompt(
    std::uint8_t choice,
    std::uint8_t joypad) noexcept;

} // namespace oasis::game::ui
