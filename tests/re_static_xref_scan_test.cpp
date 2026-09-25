#include "tools/re_static_xref_scan.hpp"

#include <cassert>
#include <cstdint>
#include <vector>

int main() {
    using namespace oasis::tools;
    std::vector<std::uint8_t> rom(0x120U, 0x4EU);
    rom[0x100] = 0x61U; rom[0x101] = 0x02U; // bsr.s 0x104
    rom[0x102] = 0x4EU; rom[0x103] = 0x75U; // rts
    rom[0x104] = 0x66U; rom[0x105] = 0x04U; // bne.s 0x10A
    rom[0x106] = 0x4EU; rom[0x107] = 0x75U; // rts

    const std::vector<StaticXrefSpan> sources{{0x100U, 0x108U}};
    const std::vector<StaticXrefSpan> targets{{0x108U, 0x10CU}};
    const auto scan = scan_static_xrefs(rom, sources, targets);
    assert(scan.decoded_instruction_count == 4U);
    assert(scan.ranges.size() == 1U);
    assert(scan.ranges.front().stop_reason == "COMPLETE");
    assert(scan.candidates.size() == 1U);
    const auto& branch = scan.candidates.front();
    assert(static_xref_kind(branch) == "DIRECT_BRANCH_TARGET");
    assert(branch.caller_pc == 0x104U);
    assert(branch.caller_end == 0x106U);
    assert(branch.target_pc == 0x10AU);
    assert(!branch.target_is_component_entry);

    const std::vector<StaticXrefSpan> call_source{{0x100U, 0x104U}};
    const std::vector<StaticXrefSpan> call_target{{0x104U, 0x106U}};
    const auto call_scan = scan_static_xrefs(rom, call_source, call_target);
    assert(call_scan.candidates.size() == 1U);
    assert(static_xref_kind(call_scan.candidates.front()) == "DIRECT_CALL_TARGET");
    assert(call_scan.candidates.front().target_is_component_entry);

    const std::vector<StaticXrefSpan> invalid_source{{0x110U, 0x112U}};
    rom[0x110] = 0xF4U; rom[0x111] = 0x00U;
    const auto stopped = scan_static_xrefs(rom, invalid_source, call_target);
    assert(stopped.decoded_instruction_count == 0U);
    assert(stopped.ranges.front().stop_reason == "UNSUPPORTED_OR_TRUNCATED_INSTRUCTION");
    return 0;
}
