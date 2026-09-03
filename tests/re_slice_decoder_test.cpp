#include "tools/re_slice_decoder.hpp"

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

int main() {
    using namespace oasis::tools;

    std::vector<std::uint8_t> synthetic(0x40, 0x4E);
    synthetic[0x00] = 0x60; synthetic[0x01] = 0x00;
    synthetic[0x02] = 0x00; synthetic[0x03] = 0x02; // branch to 0x04
    synthetic[0x04] = 0x4E; synthetic[0x05] = 0xB9;
    synthetic[0x06] = 0x00; synthetic[0x07] = 0x00;
    synthetic[0x08] = 0x00; synthetic[0x09] = 0x20; // call 0x20
    synthetic[0x0A] = 0x4E; synthetic[0x0B] = 0x75;
    synthetic[0x20] = 0x70; synthetic[0x21] = 0xFF; // moveq #-1
    synthetic[0x22] = 0x4E; synthetic[0x23] = 0x75;

    const auto slice = decode_m68k_slice(synthetic, {.entry = 0, .byte_budget = 0x30});
    assert(slice.instructions.size() == 5U);
    assert(slice.basic_blocks.size() == 3U);
    assert(slice.control_flow.size() == 2U);
    assert(slice.control_flow[0].source == 0U);
    assert(slice.control_flow[0].target == 4U);
    assert(slice.control_flow[0].kind == FlowKind::direct_branch);
    assert(slice.control_flow[1].source == 4U);
    assert(slice.control_flow[1].target == 0x20U);
    assert(slice.control_flow[1].kind == FlowKind::direct_call);
    assert(slice.instructions[3].immediate_constants.front().value == 0xFFFFFFFFU);

    const std::vector<std::uint8_t> jump_case{
        0x4E, 0xF9, 0x00, 0x00, 0x00, 0x08, // jmp $8
        0x4E, 0x71,
        0x4E, 0x75,
    };
    const auto jump_slice = decode_m68k_slice(jump_case, {.entry = 0, .byte_budget = jump_case.size()});
    assert(jump_slice.control_flow.size() == 1U);
    assert(jump_slice.control_flow.front().target == 8U);
    assert(jump_slice.control_flow.front().kind == FlowKind::direct_jump);

    std::vector<std::uint8_t> memory_case{
        0x33, 0xFC, 0x12, 0x34, 0x00, 0xFF, 0x00, 0x10, // move.w #$1234,$FF0010
        0x4E, 0x91,                                     // jsr (a1)
        0xF4, 0x00,                                     // deliberately unsupported
    };
    const auto memory_slice = decode_m68k_slice(memory_case, {.entry = 0, .byte_budget = memory_case.size()});
    assert(memory_slice.instructions.size() == 3U);
    assert(memory_slice.instructions[0].immediate_constants.front().value == 0x1234U);
    assert(memory_slice.instructions[0].memory_references.size() == 1U);
    assert(memory_slice.instructions[0].memory_references.front().address == 0x00FF0010U);
    assert(memory_slice.instructions[0].memory_references.front().kind == MemoryKind::ram);
    assert(memory_slice.unresolved_control_flow.size() == 1U);
    assert(memory_slice.unresolved_control_flow.front().kind == FlowKind::indirect_call);
    assert(memory_slice.unsupported_instruction_addresses.size() == 1U);

    const auto json = slice_to_json(memory_slice);
    assert(json.find("oasis.m68k.re-slice.v1") != std::string::npos);
    assert(json.find("0x00ff0010") != std::string::npos);
    const auto text = slice_to_text(memory_slice);
    assert(text.find("Unresolved control flow: 1") != std::string::npos);
    return 0;
}
