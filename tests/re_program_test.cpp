#include "tools/re_program.hpp"

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

int main() {
    using namespace oasis::tools;
    std::vector<std::uint8_t> synthetic(0x80, 0x4E);

    synthetic[0x00] = 0x61; synthetic[0x01] = 0x1E; // bsr 0x20
    synthetic[0x02] = 0x4E; synthetic[0x03] = 0x71; // nop
    synthetic[0x04] = 0x4E; synthetic[0x05] = 0x75; // rts
    synthetic[0x20] = 0x70; synthetic[0x21] = 0x01; // moveq #1,d0
    synthetic[0x22] = 0x4E; synthetic[0x23] = 0x75;

    synthetic[0x40] = 0x33; synthetic[0x41] = 0xFC;
    synthetic[0x42] = 0x12; synthetic[0x43] = 0x34;
    synthetic[0x44] = 0x00; synthetic[0x45] = 0xFF;
    synthetic[0x46] = 0x00; synthetic[0x47] = 0x10; // move.w #$1234,$FF0010
    synthetic[0x48] = 0x4E; synthetic[0x49] = 0x91; // jsr (a1)
    synthetic[0x4A] = 0xF4; synthetic[0x4B] = 0x00; // unsupported opcode

    synthetic[0x60] = 0x39; synthetic[0x61] = 0xC0;
    synthetic[0x62] = 0x12; synthetic[0x63] = 0x34; // invalid immediate destination
    synthetic[0x64] = 0x4E; synthetic[0x65] = 0x75;

    const std::vector<FunctionTarget> targets{
        {.entry = 0x00, .byte_budget = 0x06, .confirmed_end = 0x06},
        {.entry = 0x20, .byte_budget = 0x08, .confirmed_end = std::nullopt},
        {.entry = 0x40, .byte_budget = 0x0C, .confirmed_end = std::nullopt},
        {.entry = 0x60, .byte_budget = 0x08, .confirmed_end = std::nullopt},
    };
    const auto report = analyze_m68k_functions(synthetic, targets);
    assert(report.functions.size() == 4U);
    assert(report.functions[0].boundary == BoundaryStatus::confirmed);
    assert(report.functions[1].boundary == BoundaryStatus::discovered_return);
    assert(report.function_call_edges.size() == 1U);
    assert(report.function_call_edges.front().caller_entry == 0x00U);
    assert(report.function_call_edges.front().callee_entry == 0x20U);
    assert(report.direct_call_sites.size() == 1U);
    assert(report.direct_call_sites.front().block_start == 0x00U);
    assert(report.confirmed_memory_references.size() == 1U);
    assert(report.confirmed_memory_references.front().function_entry == 0x40U);
    assert(report.confirmed_memory_references.front().block_start == 0x40U);
    assert(report.confirmed_memory_references.front().instruction_address == 0x40U);
    assert(report.unresolved_memory_references.size() == 1U);
    assert(report.unresolved_memory_references.front().instruction_address == 0x48U);
    assert(report.unresolved_control_flow.size() == 1U);
    assert(report.unsupported_instructions.size() == 1U);
    assert(report.unsupported_addressing.size() == 1U);
    assert(report.unsupported_addressing.front().instruction_address == 0x60U);

    const auto json = program_to_json(report);
    assert(json.find("oasis.m68k.re-program.v1") != std::string::npos);
    assert(json.find("caller_to_callee") != std::string::npos);
    assert(json.find("confirmed_memory_references") != std::string::npos);
    assert(json.find("unsupported_addressing") != std::string::npos);
    const auto text = program_to_text(report);
    assert(text.find("boundary=discovered_return") != std::string::npos);
    assert(text.find("block-bound") != std::string::npos);
    return 0;
}
