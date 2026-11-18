#include "game/dialogue/dialogue_vm.hpp"

#include <algorithm>
#include <cstdint>

namespace oasis::game::dialogue {

std::uint32_t resolve_dialogue_address(
    std::span<const std::uint8_t> rom_data,
    std::uint16_t dialogue_id) noexcept {
    if (dialogue_id >= DialogueVMConfig::kStringCount) {
        return 0U;
    }
    const std::size_t entry_addr = DialogueVMConfig::kStringTableRomAddr + static_cast<std::size_t>(dialogue_id) * 2U;
    if (entry_addr + 2U > rom_data.size()) {
        return 0U;
    }

    const std::int16_t rel_offset = static_cast<std::int16_t>(
        (static_cast<std::uint16_t>(rom_data[entry_addr]) << 8U) |
        static_cast<std::uint16_t>(rom_data[entry_addr + 1U]));

    const std::int32_t target_addr = static_cast<std::int32_t>(entry_addr) + rel_offset;
    if (target_addr < 0 || static_cast<std::size_t>(target_addr) >= rom_data.size()) {
        return 0U;
    }
    return static_cast<std::uint32_t>(target_addr);
}

std::vector<DialogueToken> decode_dialogue_stream(
    std::span<const std::uint8_t> stream) noexcept {
    std::vector<DialogueToken> tokens;
    std::size_t i = 0;
    while (i < stream.size()) {
        const std::uint8_t b = stream[i++];
        if (b >= 0x20U) {
            DialogueToken tok;
            tok.is_opcode = false;
            tok.text_char = static_cast<char>(b);
            tok.op = DialogueOpcode::Terminate;
            tok.param = 0U;
            tokens.push_back(tok);
        } else {
            DialogueToken tok;
            tok.is_opcode = true;
            tok.text_char = 0;
            tok.op = static_cast<DialogueOpcode>(b);
            tok.param = 0U;
            if (tok.op == DialogueOpcode::DelayFrames && i < stream.size()) {
                tok.param = stream[i++];
            }
            tokens.push_back(tok);
            if (tok.op == DialogueOpcode::Terminate) {
                break;
            }
        }
    }
    return tokens;
}

void init_dialogue_window(
    DialogueVMState& state,
    std::uint16_t dialogue_id,
    std::uint32_t text_pc) noexcept {
    state.dialogue_id = dialogue_id;
    state.text_pc = text_pc;
    state.line_row_offsets = DialogueVMConfig::kDefaultRowOffsets;
    state.cursor_x = 0;
    state.line_idx = 0;
    state.prompt_icon = false;
    state.dismissal_wait = false;
    state.window_open = true;
    state.entity_active = true;
    state.terminated = false;
    state.delay_counter = 0U;
    state.waiting_for_button = false;
    state.pending_action = DialogueOpcode::Terminate;
    state.current_line_text.clear();
    state.completed_lines.clear();
}

void close_dialogue_window(DialogueVMState& state) noexcept {
    state.window_open = false;
    state.entity_active = false;
    state.prompt_icon = false;
    state.dismissal_wait = false;
    state.waiting_for_button = false;
    state.terminated = true;
}

void step_dialogue_vm(
    DialogueVMState& state,
    const DialogueToken& token,
    std::uint8_t joypad_pressed) noexcept {
    if (state.terminated) {
        return;
    }

    if (state.waiting_for_button) {
        if (check_dialogue_button_press(joypad_pressed)) {
            state.waiting_for_button = false;
            state.prompt_icon = false;

            switch (state.pending_action) {
            case DialogueOpcode::Terminate:
                state.dismissal_wait = false;
                close_dialogue_window(state);
                break;
            case DialogueOpcode::PromptNewline:
                state.line_idx++;
                state.cursor_x = 0;
                state.completed_lines.push_back(state.current_line_text);
                state.current_line_text.clear();
                break;
            case DialogueOpcode::PromptClearPage:
                state.line_idx = 0;
                state.cursor_x = 0;
                state.line_row_offsets = DialogueVMConfig::kDefaultRowOffsets;
                state.completed_lines.push_back(state.current_line_text);
                state.current_line_text.clear();
                break;
            case DialogueOpcode::PromptWait:
            default:
                break;
            }
        }
        return;
    }

    if (state.delay_counter > 0U) {
        state.delay_counter--;
        return;
    }

    if (!token.is_opcode) {
        state.cursor_x++;
        state.current_line_text.push_back(token.text_char);
        return;
    }

    switch (token.op) {
    case DialogueOpcode::Terminate:
        state.dismissal_wait = true;
        state.prompt_icon = true;
        state.waiting_for_button = true;
        state.pending_action = DialogueOpcode::Terminate;
        if (!state.current_line_text.empty()) {
            state.completed_lines.push_back(state.current_line_text);
            state.current_line_text.clear();
        }
        break;

    case DialogueOpcode::Newline:
        state.line_idx++;
        state.cursor_x = 0;
        state.completed_lines.push_back(state.current_line_text);
        state.current_line_text.clear();
        break;

    case DialogueOpcode::PromptNewline:
        state.prompt_icon = true;
        state.waiting_for_button = true;
        state.pending_action = DialogueOpcode::PromptNewline;
        break;

    case DialogueOpcode::DelayFrames:
        state.delay_counter = token.param;
        break;

    case DialogueOpcode::PromptWait:
        state.prompt_icon = true;
        state.waiting_for_button = true;
        state.pending_action = DialogueOpcode::PromptWait;
        break;

    case DialogueOpcode::ClearPage:
        state.line_idx = 0;
        state.cursor_x = 0;
        state.line_row_offsets = DialogueVMConfig::kDefaultRowOffsets;
        state.completed_lines.push_back(state.current_line_text);
        state.current_line_text.clear();
        break;

    case DialogueOpcode::PromptClearPage:
        state.prompt_icon = true;
        state.waiting_for_button = true;
        state.pending_action = DialogueOpcode::PromptClearPage;
        break;

    default:
        break;
    }
}

void sync_dialogue_vm_to_ram(
    const DialogueVMState& state,
    std::span<std::uint8_t> ram_window) {
    if (ram_window.size() < 0x10000U) {
        return;
    }
    constexpr auto wr_u16 = [](std::span<std::uint8_t> ram, std::uint32_t addr, std::uint16_t v) {
        const std::size_t off = addr & 0xFFFFU;
        ram[off] = static_cast<std::uint8_t>((v >> 8U) & 0xFFU);
        ram[off + 1U] = static_cast<std::uint8_t>(v & 0xFFU);
    };

    wr_u16(ram_window, DialogueVMConfig::kDialogueIdRam, state.dialogue_id);
    wr_u16(ram_window, DialogueVMConfig::kRowOffset0Ram, static_cast<std::uint16_t>(state.line_row_offsets[0]));
    wr_u16(ram_window, DialogueVMConfig::kRowOffset1Ram, static_cast<std::uint16_t>(state.line_row_offsets[1]));
    wr_u16(ram_window, DialogueVMConfig::kRowOffset2Ram, static_cast<std::uint16_t>(state.line_row_offsets[2]));
    wr_u16(ram_window, DialogueVMConfig::kRowOffset3Ram, static_cast<std::uint16_t>(state.line_row_offsets[3]));
    wr_u16(ram_window, DialogueVMConfig::kCursorXRam, static_cast<std::uint16_t>(state.cursor_x));
    wr_u16(ram_window, DialogueVMConfig::kLineIndexRam, static_cast<std::uint16_t>(state.line_idx));

    ram_window[DialogueVMConfig::kPromptWaitIconRam & 0xFFFFU] = state.prompt_icon ? 0x01U : 0x00U;
    ram_window[DialogueVMConfig::kDismissalWaitRam & 0xFFFFU] = state.dismissal_wait ? 0xFFU : 0x00U;
}

std::uint8_t read_dialogue_joypad_from_ram(
    std::span<const std::uint8_t> ram_window) noexcept {
    if (ram_window.size() < 0x10000U) {
        return 0U;
    }
    return ram_window[DialogueVMConfig::kJoypadEdgeRam & 0xFFFFU];
}

} // namespace oasis::game::dialogue
