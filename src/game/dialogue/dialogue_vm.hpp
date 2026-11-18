#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace oasis::game::dialogue {

/// Dialogue bytecode control codes (0x00C40E..0x00C534).
enum class DialogueOpcode : std::uint8_t {
    Terminate = 0x00U,        // Sets dismiss flag $FF1887, waits button, closes window
    Newline = 0x01U,          // Advances line index, 1 frame delay
    PromptNewline = 0x02U,    // Waits for button, then advances line
    DelayFrames = 0x03U,      // Reads next byte N, delays N frames
    PromptWait = 0x05U,       // Waits for button (stays on line)
    ClearPage = 0x06U,        // Scrolls 4 lines, resets row offsets and cursor
    PromptClearPage = 0x07U,  // Waits for button, then clears page
};

/// Token representation of the decoded dialogue bytecode stream.
struct DialogueToken {
    bool is_opcode{false};
    char text_char{0};
    DialogueOpcode op{DialogueOpcode::Terminate};
    std::uint8_t param{0U};
};

/// Configuration and constants for ROM Dialogue Script VM & Window Presentation Controller
/// (0x00C2EC..0x00C566, 634 bytes).
struct DialogueVMConfig {
    static constexpr std::uint32_t kEntryDisplayMsgAddr = 0x00C2ECU;
    static constexpr std::uint32_t kEntryExecWindowAddr = 0x00C326U;
    static constexpr std::uint32_t kEntryCloseWindowAddr = 0x00C45CU;
    static constexpr std::uint32_t kEntryWaitButtonAddr = 0x00C536U;
    static constexpr std::uint32_t kRangeEndAddr = 0x00C566U;
    static constexpr std::size_t kRoutineSizeBytes = 634U;

    // ROM String Table (0x051514)
    static constexpr std::uint32_t kStringTableRomAddr = 0x051514U;
    static constexpr std::size_t kStringCount = 98U; // 0x00..0x61
    static constexpr std::uint16_t kDebugSkipId1 = 0x1BU;
    static constexpr std::uint16_t kDebugSkipId2 = 0x1CU;
    static constexpr std::uint16_t kDebugSkipId3 = 0x1DU;
    static constexpr std::uint32_t kDebugMagicJoypad = 0x10100500U;

    // Joypad masks ($FF165F)
    static constexpr std::uint8_t kButtonB = 0x10U; // Bit 4
    static constexpr std::uint8_t kButtonC = 0x20U; // Bit 5
    static constexpr std::uint8_t kButtonA = 0x40U; // Bit 6
    static constexpr std::uint8_t kActionButtonsMask = 0x70U; // Bits 4, 5, 6

    // RAM Addresses
    static constexpr std::uint32_t kDialogueIdRam = 0x00FF1870U;
    static constexpr std::uint32_t kParamD6Ram = 0x00FF1872U;
    static constexpr std::uint32_t kParamD7Ram = 0x00FF1874U;
    static constexpr std::uint32_t kRowOffset0Ram = 0x00FF1876U;
    static constexpr std::uint32_t kRowOffset1Ram = 0x00FF1878U;
    static constexpr std::uint32_t kRowOffset2Ram = 0x00FF187AU;
    static constexpr std::uint32_t kRowOffset3Ram = 0x00FF187CU;
    static constexpr std::uint32_t kCursorXRam = 0x00FF187EU;
    static constexpr std::uint32_t kLineIndexRam = 0x00FF1880U;
    static constexpr std::uint32_t kParamD5Ram = 0x00FF1882U;
    static constexpr std::uint32_t kPromptWaitIconRam = 0x00FF1886U;
    static constexpr std::uint32_t kDismissalWaitRam = 0x00FF1887U;
    static constexpr std::uint32_t kEntitySlotPtrRam = 0x00FF186CU;
    static constexpr std::uint32_t kJoypadEdgeRam = 0x00FF165FU;
    static constexpr std::uint32_t kJoypadHeldRam = 0x00FF165EU;
    static constexpr std::uint32_t kDebugFlagRam = 0x00FF0D7AU;

    // Default line row offsets (12, 24, 36, 48)
    static constexpr std::array<std::int16_t, 4> kDefaultRowOffsets = {12, 24, 36, 48};
};

/// Full state of the Dialogue Script Virtual Machine.
struct DialogueVMState {
    std::uint16_t dialogue_id{0U};
    std::uint32_t text_pc{0U};
    std::array<std::int16_t, 4> line_row_offsets{DialogueVMConfig::kDefaultRowOffsets};
    std::int16_t cursor_x{0};
    std::int16_t line_idx{0};
    bool prompt_icon{false};
    bool dismissal_wait{false};
    bool window_open{false};
    bool entity_active{false};
    bool terminated{false};
    std::uint8_t delay_counter{0U};
    bool waiting_for_button{false};
    DialogueOpcode pending_action{DialogueOpcode::Terminate};
    std::string current_line_text{};
    std::vector<std::string> completed_lines{};
};

/// Resolves target ROM address of a string from table at 0x051514 using 16-bit relative signed offset.
[[nodiscard]] std::uint32_t resolve_dialogue_address(
    std::span<const std::uint8_t> rom_data,
    std::uint16_t dialogue_id) noexcept;

/// Decodes raw dialogue bytecode stream into token sequence until Terminate (0x00) or end of stream.
[[nodiscard]] std::vector<DialogueToken> decode_dialogue_stream(
    std::span<const std::uint8_t> stream) noexcept;

/// Tests if joypad input matches any of action buttons A, B, or C (mask 0x70).
[[nodiscard]] constexpr bool check_dialogue_button_press(std::uint8_t joypad_pressed) noexcept {
    return (joypad_pressed & DialogueVMConfig::kActionButtonsMask) != 0U;
}

/// Evaluates debug dialogue test trigger conditions (0x00C2EC).
[[nodiscard]] constexpr bool is_debug_dialogue_test_requested(
    std::uint8_t flag_ff0d7a,
    std::uint32_t joypad_held) noexcept {
    return ((flag_ff0d7a & 0x80U) != 0U) && (joypad_held == DialogueVMConfig::kDebugMagicJoypad);
}

/// Checks if dialogue id is in the debug skip set (0x1B, 0x1C, 0x1D).
[[nodiscard]] constexpr bool should_skip_debug_dialogue_id(std::uint16_t id) noexcept {
    return id == DialogueVMConfig::kDebugSkipId1 ||
           id == DialogueVMConfig::kDebugSkipId2 ||
           id == DialogueVMConfig::kDebugSkipId3;
}

/// Initializes the VM state for executing a window (0x00C326).
void init_dialogue_window(
    DialogueVMState& state,
    std::uint16_t dialogue_id,
    std::uint32_t text_pc = 0U) noexcept;

/// Closes the dialogue window, clearing entity and state flags (0x00C45C).
void close_dialogue_window(DialogueVMState& state) noexcept;

/// Steps the dialogue virtual machine with the given token and frame joypad input.
void step_dialogue_vm(
    DialogueVMState& state,
    const DialogueToken& token,
    std::uint8_t joypad_pressed = 0U) noexcept;

/// Synchronizes Dialogue VM state to a 64KB Genesis RAM window.
void sync_dialogue_vm_to_ram(
    const DialogueVMState& state,
    std::span<std::uint8_t> ram_window);

/// Reads joypad and trigger state from 64KB Genesis RAM window.
[[nodiscard]] std::uint8_t read_dialogue_joypad_from_ram(
    std::span<const std::uint8_t> ram_window) noexcept;

} // namespace oasis::game::dialogue
