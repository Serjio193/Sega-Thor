#include "tools/re_slice_decoder.hpp"

#include <cassert>
#include <cstdint>
#include <vector>

int main() {
    using namespace oasis::tools;
    std::vector<std::uint8_t> rom(0x2D84, 0x4E);
    const std::vector<std::uint8_t> routine{
        0x48, 0xE7, 0x01, 0x10, 0x42, 0x47, 0x1E, 0x1E,
        0x47, 0xF9, 0x00, 0xFF, 0x13, 0x4C, 0xD6, 0xC7,
        0x1E, 0x1E, 0x36, 0xDE, 0x51, 0xCF, 0xFF, 0xFC,
        0x4C, 0xDF, 0x08, 0x80, 0x4E, 0x75};
    for (std::size_t i = 0; i < routine.size(); ++i) rom[0x2D66 + i] = routine[i];
    const auto slice = decode_m68k_slice(rom, {.entry = 0x2D66, .byte_budget = 0x1E});
    assert(slice.entry == 0x2D66);
    assert(slice.range_end == 0x2D84);
    assert(slice.instructions.size() == 10U);
    assert(slice.basic_blocks.size() == 3U);
    assert(slice.control_flow.size() == 1U);
    assert(slice.control_flow.front().source == 0x2D7A);
    assert(slice.control_flow.front().target == 0x2D78);
    assert(slice.control_flow.front().kind == FlowKind::direct_branch);
    assert(slice.unresolved_control_flow.empty());
    assert(slice.unsupported_instruction_addresses.empty());
    assert(slice.instructions.front().flow == FlowKind::none);
    assert(slice.instructions.back().flow == FlowKind::return_instruction);
    return 0;
}
