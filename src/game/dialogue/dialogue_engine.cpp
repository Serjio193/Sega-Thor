#include "game/dialogue/dialogue_engine.hpp"

namespace oasis::game::dialogue {

std::optional<std::uint32_t> lookup_dialogue_rom_addr(
    std::span<const std::uint8_t> rom,
    std::uint16_t dialogue_id) noexcept {
    if (dialogue_id >= DialogueEngineConfig::kTableEntryCount) {
        return std::nullopt;
    }
    const std::size_t entry_addr =
        static_cast<std::size_t>(DialogueEngineConfig::kTableRomAddr) +
        static_cast<std::size_t>(dialogue_id) * 2U;
    if (entry_addr + 2U > rom.size()) {
        return std::nullopt;
    }
    const std::uint16_t rel = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(rom[entry_addr]) << 8U) |
        static_cast<std::uint16_t>(rom[entry_addr + 1U]));
    const std::size_t target = entry_addr + static_cast<std::size_t>(rel);
    if (target >= rom.size()) {
        return std::nullopt;
    }
    return static_cast<std::uint32_t>(target);
}

std::optional<std::size_t> glyph_rom_offset(char ch) noexcept {
    const auto u = static_cast<std::uint8_t>(ch);
    if (u < DialogueEngineConfig::kAsciiBase) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(u - DialogueEngineConfig::kAsciiBase) *
           DialogueEngineConfig::kGlyphStrideBytes;
}

void DialogueEngine::reset() noexcept {
    state_ = DialogueEngineState{};
}

bool DialogueEngine::start(std::span<const std::uint8_t> rom, std::uint16_t dialogue_id) noexcept {
    const auto addr = lookup_dialogue_rom_addr(rom, dialogue_id);
    if (!addr.has_value()) {
        reset();
        return false;
    }
    start_at(*addr, dialogue_id);
    return true;
}

void DialogueEngine::start_at(std::uint32_t text_pc, std::uint16_t dialogue_id) noexcept {
    reset();
    state_.dialogue_id = dialogue_id;
    state_.text_pc = text_pc;
    state_.box_open = true;
}

void DialogueEngine::advance_line() noexcept {
    state_.col_cursor = 0U;
    if (state_.line_cursor < 3U) {
        ++state_.line_cursor;
    } else {
        // Scroll 4-line window upward by 1 line (0x00C566)
        state_.emitted_lines[0] = state_.emitted_lines[1];
        state_.emitted_lines[1] = state_.emitted_lines[2];
        state_.emitted_lines[2] = state_.emitted_lines[3];
        state_.emitted_lines[3].clear();
    }
}

void DialogueEngine::clear_page() noexcept {
    state_.col_cursor = 0U;
    state_.line_cursor = 0U;
    state_.line_vram_offsets = DialogueEngineConfig::kDefaultLineVramOffsets;
    for (auto& line : state_.emitted_lines) {
        line.clear();
    }
}

bool DialogueEngine::step_frame(std::span<const std::uint8_t> rom, bool button_pressed) {
    if (!state_.box_open || state_.finished) {
        return false;
    }

    if (state_.delay_frames > 0U) {
        --state_.delay_frames;
        if (state_.delay_frames > 0U) {
            return true;
        }
    }

    if (state_.waiting_button) {
        if (!button_pressed) {
            return true;
        }
        state_.waiting_button = false;
        const PendingWaitAction action = state_.wait_action;
        state_.wait_action = PendingWaitAction::None;
        switch (action) {
            case PendingWaitAction::EndClose:
                state_.end_flag_ff1887 = 0U;
                state_.box_open = false;
                state_.finished = true;
                return false;
            case PendingWaitAction::Newline:
                advance_line();
                return true;
            case PendingWaitAction::ClearPage:
                clear_page();
                return true;
            case PendingWaitAction::ContinueOnly:
            case PendingWaitAction::None:
                break;
        }
    }

    while (state_.text_pc < rom.size()) {
        const std::uint8_t byte = rom[state_.text_pc++];
        if (byte >= DialogueEngineConfig::kAsciiBase) {
            if (const auto off = glyph_rom_offset(static_cast<char>(byte)); off.has_value()) {
                state_.emitted_glyph_offsets.push_back(*off);
            }
            state_.emitted_lines[state_.line_cursor].push_back(static_cast<char>(byte));
            ++state_.col_cursor;
            continue;
        }

        switch (static_cast<DialogueControlCode>(byte)) {
            case DialogueControlCode::End: // 0x00 -> 0x00C4AA
                state_.end_flag_ff1887 = 0xFFU;
                state_.waiting_button = true;
                state_.wait_action = PendingWaitAction::EndClose;
                return true;
            case DialogueControlCode::Newline: // 0x01 -> 0x00C4BC
                advance_line();
                return true;
            case DialogueControlCode::WaitButtonNewline: // 0x02 -> 0x00C4C8
                state_.waiting_button = true;
                state_.wait_action = PendingWaitAction::Newline;
                return true;
            case DialogueControlCode::DelayFrames: // 0x03 -> 0x00C4CE
                if (state_.text_pc < rom.size()) {
                    state_.delay_frames = rom[state_.text_pc++];
                }
                return true;
            case DialogueControlCode::WaitButton: // 0x05 -> 0x00C4E4
                state_.waiting_button = true;
                state_.wait_action = PendingWaitAction::ContinueOnly;
                return true;
            case DialogueControlCode::ClearPage: // 0x06 -> 0x00C4EC
                clear_page();
                return true;
            case DialogueControlCode::WaitButtonClearPage: // 0x07 -> 0x00C530
                state_.waiting_button = true;
                state_.wait_action = PendingWaitAction::ClearPage;
                return true;
            default:
                break;
        }
    }

    state_.box_open = false;
    state_.finished = true;
    return false;
}

} // namespace oasis::game::dialogue
