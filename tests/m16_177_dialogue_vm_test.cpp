#include "game/dialogue/dialogue_vm.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace oasis::game::dialogue;

static void test_constants_and_addresses() {
    static_assert(DialogueVMConfig::kEntryDisplayMsgAddr == 0x00C2ECU);
    static_assert(DialogueVMConfig::kEntryExecWindowAddr == 0x00C326U);
    static_assert(DialogueVMConfig::kEntryCloseWindowAddr == 0x00C45CU);
    static_assert(DialogueVMConfig::kEntryWaitButtonAddr == 0x00C536U);
    static_assert(DialogueVMConfig::kRangeEndAddr == 0x00C566U);
    static_assert(DialogueVMConfig::kRoutineSizeBytes == 634U);

    static_assert(DialogueVMConfig::kStringTableRomAddr == 0x051514U);
    static_assert(DialogueVMConfig::kStringCount == 98U);
    static_assert(DialogueVMConfig::kDebugSkipId1 == 0x1BU);
    static_assert(DialogueVMConfig::kDebugSkipId2 == 0x1CU);
    static_assert(DialogueVMConfig::kDebugSkipId3 == 0x1DU);
    static_assert(DialogueVMConfig::kDebugMagicJoypad == 0x10100500U);

    static_assert(DialogueVMConfig::kButtonB == 0x10U);
    static_assert(DialogueVMConfig::kButtonC == 0x20U);
    static_assert(DialogueVMConfig::kButtonA == 0x40U);
    static_assert(DialogueVMConfig::kActionButtonsMask == 0x70U);

    static_assert(DialogueVMConfig::kDialogueIdRam == 0x00FF1870U);
    static_assert(DialogueVMConfig::kPromptWaitIconRam == 0x00FF1886U);
    static_assert(DialogueVMConfig::kDismissalWaitRam == 0x00FF1887U);
    static_assert(DialogueVMConfig::kJoypadEdgeRam == 0x00FF165FU);

    std::cout << "[PASS] test_constants_and_addresses\n";
}

static void test_string_table_resolution() {
    // Construct synthetic ROM buffer around 0x051514
    std::vector<std::uint8_t> rom(0x052000U, 0x00U);
    constexpr std::size_t table_base = DialogueVMConfig::kStringTableRomAddr;

    // Entry 0: relative offset +200 (0x00C8) -> target = table_base + 0 + 200 = table_base + 200
    rom[table_base + 0U] = 0x00U;
    rom[table_base + 1U] = 0xC8U;

    const std::uint32_t addr0 = resolve_dialogue_address(rom, 0U);
    assert(addr0 == table_base + 200U);

    // Entry 5: relative offset +300 (0x012C) -> target = table_base + 10 + 300 = table_base + 310
    rom[table_base + 10U] = 0x01U;
    rom[table_base + 11U] = 0x2CU;

    const std::uint32_t addr5 = resolve_dialogue_address(rom, 5U);
    assert(addr5 == table_base + 310U);

    // Out of bounds ID (>= 98) returns 0
    assert(resolve_dialogue_address(rom, 98U) == 0U);
    assert(resolve_dialogue_address(rom, 200U) == 0U);

    // Truncated ROM buffer returns 0
    std::vector<std::uint8_t> tiny_rom(table_base + 4U, 0x00U);
    assert(resolve_dialogue_address(tiny_rom, 10U) == 0U);

    std::cout << "[PASS] test_string_table_resolution\n";
}

static void test_stream_decoder() {
    // Stream: "Hi!\x01Line2\x03\x05AfterDelay\x00IgnoredTrailing"
    const std::vector<std::uint8_t> stream = {
        'H', 'i', '!',
        0x01U, // Newline
        'L', 'i', 'n', 'e', '2',
        0x03U, 0x05U, // Delay 5 frames
        'A', 'f', 't', 'e', 'r',
        0x00U, // Terminate
        'I', 'g', 'n', 'o', 'r', 'e', 'd'
    };

    const std::vector<DialogueToken> tokens = decode_dialogue_stream(stream);

    // Verify token count: 'H', 'i', '!', Newline, 'L', 'i', 'n', 'e', '2', Delay, 'A', 'f', 't', 'e', 'r', Terminate
    assert(tokens.size() == 16U);

    assert(!tokens[0].is_opcode && tokens[0].text_char == 'H');
    assert(!tokens[1].is_opcode && tokens[1].text_char == 'i');
    assert(!tokens[2].is_opcode && tokens[2].text_char == '!');

    assert(tokens[3].is_opcode && tokens[3].op == DialogueOpcode::Newline);

    assert(!tokens[4].is_opcode && tokens[4].text_char == 'L');
    assert(tokens[8].text_char == '2');

    assert(tokens[9].is_opcode && tokens[9].op == DialogueOpcode::DelayFrames && tokens[9].param == 5U);

    assert(!tokens[10].is_opcode && tokens[10].text_char == 'A');
    assert(tokens[14].text_char == 'r');

    assert(tokens[15].is_opcode && tokens[15].op == DialogueOpcode::Terminate);

    std::cout << "[PASS] test_stream_decoder\n";
}

static void test_vm_lifecycle_and_text_emission() {
    DialogueVMState vm{};
    init_dialogue_window(vm, 42U, 0x051600U);

    assert(vm.dialogue_id == 42U);
    assert(vm.window_open);
    assert(vm.entity_active);
    assert(!vm.terminated);
    assert(vm.cursor_x == 0);
    assert(vm.line_idx == 0);

    // Emit "Ali"
    DialogueToken t_a{.is_opcode = false, .text_char = 'A'};
    DialogueToken t_l{.is_opcode = false, .text_char = 'l'};
    DialogueToken t_i{.is_opcode = false, .text_char = 'i'};
    step_dialogue_vm(vm, t_a);
    step_dialogue_vm(vm, t_l);
    step_dialogue_vm(vm, t_i);

    assert(vm.cursor_x == 3);
    assert(vm.current_line_text == "Ali");

    // Newline
    DialogueToken t_nl{.is_opcode = true, .op = DialogueOpcode::Newline};
    step_dialogue_vm(vm, t_nl);

    assert(vm.line_idx == 1);
    assert(vm.cursor_x == 0);
    assert(vm.completed_lines.size() == 1U);
    assert(vm.completed_lines[0] == "Ali");
    assert(vm.current_line_text.empty());

    // Terminate
    DialogueToken t_term{.is_opcode = true, .op = DialogueOpcode::Terminate};
    step_dialogue_vm(vm, t_term);

    assert(vm.dismissal_wait);
    assert(vm.prompt_icon);
    assert(vm.waiting_for_button);
    assert(vm.window_open);

    // Frame step with no buttons pressed: remains waiting
    step_dialogue_vm(vm, t_term, 0x00U);
    assert(vm.waiting_for_button);
    assert(vm.window_open);

    // Button A pressed (0x40)
    step_dialogue_vm(vm, t_term, DialogueVMConfig::kButtonA);
    assert(!vm.waiting_for_button);
    assert(!vm.prompt_icon);
    assert(!vm.dismissal_wait);
    assert(!vm.window_open);
    assert(!vm.entity_active);
    assert(vm.terminated);

    std::cout << "[PASS] test_vm_lifecycle_and_text_emission\n";
}

static void test_joypad_action_buttons_and_masks() {
    // Action buttons: B (0x10), C (0x20), A (0x40)
    assert(check_dialogue_button_press(0x10U)); // Button B
    assert(check_dialogue_button_press(0x20U)); // Button C
    assert(check_dialogue_button_press(0x40U)); // Button A
    assert(check_dialogue_button_press(0x70U)); // All three
    assert(check_dialogue_button_press(0x50U)); // A + B

    // Non-action buttons (D-pad, Start)
    assert(!check_dialogue_button_press(0x00U));
    assert(!check_dialogue_button_press(0x01U)); // Up
    assert(!check_dialogue_button_press(0x02U)); // Down
    assert(!check_dialogue_button_press(0x04U)); // Left
    assert(!check_dialogue_button_press(0x08U)); // Right
    assert(!check_dialogue_button_press(0x80U)); // Start

    std::cout << "[PASS] test_joypad_action_buttons_and_masks\n";
}

static void test_prompt_opcodes() {
    // Test PromptNewline (0x02)
    {
        DialogueVMState vm{};
        init_dialogue_window(vm, 1U);
        DialogueToken tok_prompt_nl{.is_opcode = true, .op = DialogueOpcode::PromptNewline};
        step_dialogue_vm(vm, tok_prompt_nl);

        assert(vm.prompt_icon);
        assert(vm.waiting_for_button);
        assert(vm.line_idx == 0);

        // Press Button C (0x20)
        step_dialogue_vm(vm, tok_prompt_nl, DialogueVMConfig::kButtonC);
        assert(!vm.prompt_icon);
        assert(!vm.waiting_for_button);
        assert(vm.line_idx == 1);
    }

    // Test PromptClearPage (0x07)
    {
        DialogueVMState vm{};
        init_dialogue_window(vm, 2U);
        vm.line_idx = 3;
        vm.cursor_x = 15;

        DialogueToken tok_prompt_clear{.is_opcode = true, .op = DialogueOpcode::PromptClearPage};
        step_dialogue_vm(vm, tok_prompt_clear);

        assert(vm.prompt_icon);
        assert(vm.waiting_for_button);
        assert(vm.line_idx == 3);

        // Press Button B (0x10)
        step_dialogue_vm(vm, tok_prompt_clear, DialogueVMConfig::kButtonB);
        assert(!vm.prompt_icon);
        assert(!vm.waiting_for_button);
        assert(vm.line_idx == 0);
        assert(vm.cursor_x == 0);
    }

    // Test PromptWait (0x05)
    {
        DialogueVMState vm{};
        init_dialogue_window(vm, 3U);
        vm.cursor_x = 8;

        DialogueToken tok_wait{.is_opcode = true, .op = DialogueOpcode::PromptWait};
        step_dialogue_vm(vm, tok_wait);

        assert(vm.prompt_icon);
        assert(vm.waiting_for_button);

        // Press Button A (0x40)
        step_dialogue_vm(vm, tok_wait, DialogueVMConfig::kButtonA);
        assert(!vm.prompt_icon);
        assert(!vm.waiting_for_button);
        assert(vm.cursor_x == 8); // Preserves cursor position
    }

    std::cout << "[PASS] test_prompt_opcodes\n";
}

static void test_delay_frames_opcode() {
    DialogueVMState vm{};
    init_dialogue_window(vm, 0U);

    DialogueToken tok_delay{.is_opcode = true, .op = DialogueOpcode::DelayFrames, .param = 3U};
    step_dialogue_vm(vm, tok_delay);

    assert(vm.delay_counter == 3U);

    DialogueToken tok_char{.is_opcode = false, .text_char = 'X'};
    step_dialogue_vm(vm, tok_char); // Consumes 1 delay frame
    assert(vm.delay_counter == 2U);
    assert(vm.current_line_text.empty());

    step_dialogue_vm(vm, tok_char); // Consumes 1 delay frame
    assert(vm.delay_counter == 1U);
    assert(vm.current_line_text.empty());

    step_dialogue_vm(vm, tok_char); // Consumes 1 delay frame (hits 0)
    assert(vm.delay_counter == 0U);
    assert(vm.current_line_text.empty());

    // Next step executes the token
    step_dialogue_vm(vm, tok_char);
    assert(vm.current_line_text == "X");

    std::cout << "[PASS] test_delay_frames_opcode\n";
}

static void test_debug_mode_conditions() {
    assert(is_debug_dialogue_test_requested(0x80U, 0x10100500U));
    assert(!is_debug_dialogue_test_requested(0x00U, 0x10100500U));
    assert(!is_debug_dialogue_test_requested(0x80U, 0x00000000U));
    assert(!is_debug_dialogue_test_requested(0x7FU, 0x10100500U));

    assert(should_skip_debug_dialogue_id(0x1BU));
    assert(should_skip_debug_dialogue_id(0x1CU));
    assert(should_skip_debug_dialogue_id(0x1DU));
    assert(!should_skip_debug_dialogue_id(0x00U));
    assert(!should_skip_debug_dialogue_id(0x1AU));
    assert(!should_skip_debug_dialogue_id(0x1EU));

    std::cout << "[PASS] test_debug_mode_conditions\n";
}

static void test_ram_window_sync() {
    std::vector<std::uint8_t> ram(0x10000U, 0x00U);

    DialogueVMState vm{};
    init_dialogue_window(vm, 0x0017U);
    vm.cursor_x = 42;
    vm.line_idx = 2;
    vm.prompt_icon = true;
    vm.dismissal_wait = true;

    sync_dialogue_vm_to_ram(vm, ram);

    // Verify dialogue id
    constexpr std::size_t id_off = DialogueVMConfig::kDialogueIdRam & 0xFFFFU;
    const std::uint16_t id_val = static_cast<std::uint16_t>((ram[id_off] << 8U) | ram[id_off + 1U]);
    assert(id_val == 0x0017U);

    // Verify cursor X
    constexpr std::size_t cur_off = DialogueVMConfig::kCursorXRam & 0xFFFFU;
    const std::uint16_t cur_val = static_cast<std::uint16_t>((ram[cur_off] << 8U) | ram[cur_off + 1U]);
    assert(cur_val == 42U);

    // Verify line index
    constexpr std::size_t line_off = DialogueVMConfig::kLineIndexRam & 0xFFFFU;
    const std::uint16_t line_val = static_cast<std::uint16_t>((ram[line_off] << 8U) | ram[line_off + 1U]);
    assert(line_val == 2U);

    // Verify prompt icon & dismissal wait flags
    assert(ram[DialogueVMConfig::kPromptWaitIconRam & 0xFFFFU] == 0x01U);
    assert(ram[DialogueVMConfig::kDismissalWaitRam & 0xFFFFU] == 0xFFU);

    // Test joypad read
    ram[DialogueVMConfig::kJoypadEdgeRam & 0xFFFFU] = 0x60U; // Buttons A and C
    assert(read_dialogue_joypad_from_ram(ram) == 0x60U);

    std::cout << "[PASS] test_ram_window_sync\n";
}

int main() {
    std::cout << "Starting M16.177 Dialogue Script VM tests...\n";
    test_constants_and_addresses();
    test_string_table_resolution();
    test_stream_decoder();
    test_vm_lifecycle_and_text_emission();
    test_joypad_action_buttons_and_masks();
    test_prompt_opcodes();
    test_delay_frames_opcode();
    test_debug_mode_conditions();
    test_ram_window_sync();
    std::cout << "All M16.177 Dialogue Script VM tests PASS!\n";
    return 0;
}
