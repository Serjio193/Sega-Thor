#include "game/dialogue/dialogue_engine.hpp"

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

namespace {

void write_be16(std::vector<std::uint8_t>& buf, std::size_t off, std::uint16_t v) {
    buf[off] = static_cast<std::uint8_t>((v >> 8U) & 0xFFU);
    buf[off + 1U] = static_cast<std::uint8_t>(v & 0xFFU);
}

void write_text(std::vector<std::uint8_t>& buf, std::size_t off, const std::vector<std::uint8_t>& bytes) {
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        buf[off + i] = bytes[i];
    }
}

} // namespace

int main() {
    using oasis::game::dialogue::DialogueEngine;
    using oasis::game::dialogue::DialogueEngineConfig;
    using oasis::game::dialogue::glyph_rom_offset;
    using oasis::game::dialogue::lookup_dialogue_rom_addr;

    // 1. Font glyph offset formula: (ch - 0x20) * 32.
    assert(glyph_rom_offset(' ') == 0U);
    assert(glyph_rom_offset('A') == (0x41U - 0x20U) * 32U);
    assert(!glyph_rom_offset('\0').has_value());

    // 2. Synthetic ROM with 0x051514 table (id=0 -> +232 = 0x0515FC, id=1 -> +438 = 0x0516CC).
    std::vector<std::uint8_t> rom(0x052000U, 0x00U);
    write_be16(rom, DialogueEngineConfig::kTableRomAddr + 0U * 2U, 232U);
    write_be16(rom, DialogueEngineConfig::kTableRomAddr + 1U * 2U, 438U);

    assert(lookup_dialogue_rom_addr(rom, 0U) == 0x000515FCU);
    assert(lookup_dialogue_rom_addr(rom, 1U) == 0x000516CCU);
    assert(!lookup_dialogue_rom_addr(rom, 116U).has_value());

    // Script at 0x0515FC testing all 7 control codes:
    // "Hi" + Newline(0x01) + "Ali" + DelayFrames(0x03, 2) + WaitBtn(0x05) + "!" + WaitBtnClear(0x07) + "End" + End(0x00)
    write_text(rom, 0x0515FCU, {
        'H', 'i', 0x01U,
        'A', 'l', 'i', 0x03U, 0x02U,
        0x05U, '!', 0x07U,
        'E', 'n', 'd', 0x00U,
    });

    DialogueEngine engine;
    assert(engine.start(rom, 0U));
    assert(engine.state().dialogue_id == 0U);
    assert(engine.state().line_vram_offsets == DialogueEngineConfig::kDefaultLineVramOffsets);

    // Frame 1: emits "Hi" on line 0, hits 0x01 Newline -> line_cursor == 1.
    assert(engine.step_frame(rom));
    assert(engine.state().emitted_lines[0] == "Hi");
    assert(engine.state().line_cursor == 1U);
    assert(engine.state().col_cursor == 0U);

    // Frame 2: emits "Ali" on line 1, hits 0x03 DelayFrames(2).
    assert(engine.step_frame(rom));
    assert(engine.state().emitted_lines[1] == "Ali");
    assert(engine.state().delay_frames == 2U);

    // Frame 3: delay_frames 2 -> 1 (yields).
    assert(engine.step_frame(rom));
    assert(engine.state().delay_frames == 1U);

    // Frame 4: delay_frames 1 -> 0, hits 0x05 WaitButton.
    assert(engine.step_frame(rom));
    assert(engine.state().waiting_button);

    // Frame 5 (no button): still waiting.
    assert(engine.step_frame(rom, false));
    assert(engine.state().waiting_button);

    // Frame 6 (button pressed): emits "!", hits 0x07 WaitButtonClearPage.
    assert(engine.step_frame(rom, true));
    assert(engine.state().emitted_lines[1] == "Ali!");
    assert(engine.state().waiting_button);

    // Frame 7 (button pressed): clears page.
    assert(engine.step_frame(rom, true));
    assert(engine.state().line_cursor == 0U);
    assert(engine.state().emitted_lines[0].empty());

    // Frame 8: emits "End", hits 0x00 End -> $FF1887 = 0xFF, waiting_button.
    assert(engine.step_frame(rom));
    assert(engine.state().emitted_lines[0] == "End");
    assert(engine.state().end_flag_ff1887 == 0xFFU);
    assert(engine.state().waiting_button);

    // Frame 9 (button pressed): closes box and finishes.
    assert(!engine.step_frame(rom, true));
    assert(engine.state().finished);
    assert(!engine.state().box_open);
    assert(engine.state().end_flag_ff1887 == 0U);

    return 0;
}
