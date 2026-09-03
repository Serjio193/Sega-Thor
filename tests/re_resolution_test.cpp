#include "tools/re_resolution.hpp"

#include <cassert>
#include <cstdint>
#include <string>

namespace {

using namespace oasis::tools;

DecodedInstruction instruction(std::uint32_t address, std::uint16_t opcode,
                               std::initializer_list<std::uint8_t> bytes,
                               const char* mnemonic = "unary") {
    return {.address = address, .opcode = opcode, .bytes = bytes, .mnemonic = mnemonic, .supported = true};
}

AtlasEntry atlas_entry(std::initializer_list<AtlasUnresolvedReference> refs) {
    AtlasEntry entry;
    entry.start = 0x1000;
    entry.unresolved_references = refs;
    return entry;
}

DecodedSlice one_block(std::vector<DecodedInstruction> instructions) {
    DecodedSlice slice;
    slice.entry = 0x1000;
    slice.range_end = instructions.back().address + static_cast<std::uint32_t>(instructions.back().bytes.size());
    BasicBlock block{0x1000, slice.range_end, {}};
    for (const auto& item : instructions) block.instruction_addresses.push_back(item.address);
    slice.instructions = std::move(instructions);
    slice.basic_blocks.push_back(std::move(block));
    return slice;
}

AtlasUnresolvedReference ref(std::uint32_t address, std::uint8_t base) {
    return {address, 0x1000, 5, base, "address_displacement", "unary", "register_based", false, false};
}

void test_local_propagation() {
    auto slice = one_block({
        instruction(0x1000, 0x207C, {0x20, 0x7C, 0x00, 0x00, 0x10, 0x00}, "move"),
        instruction(0x1006, 0x4A28, {0x4A, 0x28, 0x00, 0x04}),
        instruction(0x100A, 0x2248, {0x22, 0x48}, "move"),
        instruction(0x100C, 0x4A29, {0x4A, 0x29, 0xFF, 0xF0}),
        instruction(0x1010, 0xD1FC, {0xD1, 0xFC, 0x00, 0x00, 0x00, 0x04}, "binary"),
        instruction(0x1016, 0x4A28, {0x4A, 0x28, 0x00, 0x02}),
    });
    const auto report = resolve_decoded_displacements(atlas_entry({
        ref(0x1006, 0), ref(0x100C, 1), ref(0x1016, 0)}), slice);
    assert(report.newly_resolved == 3);
    assert(report.items[0].effective_address == 0x1004);
    assert(report.items[1].effective_address == 0x0FF0);
    assert(report.items[2].effective_address == 0x1006);
    assert(report.items[2].provenance.size() >= 2);
}

void test_unknown_and_unsupported_transfer() {
    auto slice = one_block({
        instruction(0x1000, 0x43E8, {0x43, 0xE8, 0x00, 0x02}, "lea"),
        instruction(0x1004, 0x4A29, {0x4A, 0x29, 0x00, 0x02}),
        instruction(0x1008, 0x4A2A, {0x4A, 0x2A, 0x00, 0x02}),
    });
    const auto report = resolve_decoded_displacements(atlas_entry({ref(0x1004, 1), ref(0x1008, 2)}), slice);
    assert(report.items[0].status == ResolutionStatus::unresolved_unknown_base);
    assert(report.items[1].status == ResolutionStatus::unresolved_unknown_base);

    auto ambiguous = one_block({
        instruction(0x1000, 0x1168, {0x11, 0x68, 0x00, 0x02, 0x00, 0x04}, "move"),
    });
    const auto ambiguous_report = resolve_decoded_displacements(atlas_entry({ref(0x1000, 0)}), ambiguous);
    assert(ambiguous_report.items[0].status == ResolutionStatus::unresolved_unsupported_transfer);
}

ResolutionReport merge_case(bool identical) {
    DecodedSlice slice;
    slice.entry = 0x1000;
    slice.range_end = 0x1028;
    slice.instructions = {
        instruction(0x1000, 0x6600, {0x66, 0x00, 0x00, 0x0E}, "bcc"),
        instruction(0x1004, 0x207C, {0x20, 0x7C, 0x00, 0x00, 0x10, 0x00}, "move"),
        instruction(0x100A, 0x6000, {0x60, 0x00, 0x00, 0x10}, "bra"),
        instruction(0x1010, 0x207C, {0x20, 0x7C, 0x00, 0x00, static_cast<std::uint8_t>(identical ? 0x10 : 0x20), 0x00}, "move"),
        instruction(0x1016, 0x6000, {0x60, 0x00, 0x00, 0x08}, "bra"),
        instruction(0x1020, 0x4A28, {0x4A, 0x28, 0x00, 0x02}),
    };
    slice.basic_blocks = {
        {0x1000, 0x1004, {0x1000}}, {0x1004, 0x1010, {0x1004, 0x100A}},
        {0x1010, 0x101C, {0x1010, 0x1016}}, {0x1020, 0x1024, {0x1020}},
    };
    slice.control_flow = {{0x1000, 0x1010, FlowKind::direct_branch},
                          {0x100A, 0x1020, FlowKind::direct_branch},
                          {0x1016, 0x1020, FlowKind::direct_branch}};
    return resolve_decoded_displacements(atlas_entry({ref(0x1020, 0)}), slice);
}

} // namespace

int main() {
    test_local_propagation();
    test_unknown_and_unsupported_transfer();
    assert(merge_case(false).items[0].status == ResolutionStatus::unresolved_cfg_merge);
    assert(merge_case(true).items[0].status == ResolutionStatus::resolved);
    const auto json = resolution_to_json(merge_case(true));
    const auto text = resolution_to_text(merge_case(true));
    assert(json.find("oasis.m68k.re-resolution.v1") != std::string::npos);
    assert(text.find("provenance_examples") != std::string::npos);
    return 0;
}
