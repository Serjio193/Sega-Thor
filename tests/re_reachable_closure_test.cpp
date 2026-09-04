#include "tools/re_reachable_closure.hpp"

#include <cassert>
#include <cstdint>
#include <initializer_list>
#include <string>

namespace {

using namespace oasis::tools;

DecodedInstruction instruction(std::uint32_t address, std::uint16_t opcode,
                               std::initializer_list<std::uint8_t> bytes,
                               const char* mnemonic = "move") {
    return {.address = address, .opcode = opcode, .bytes = bytes, .mnemonic = mnemonic, .supported = true};
}

AtlasUnresolvedReference reference(std::uint32_t address, std::uint8_t base) {
    return {address, address, 5, base, "address_displacement", "memory", "register_based", false, false};
}

DecodedSlice linear(std::vector<DecodedInstruction> instructions) {
    DecodedSlice slice;
    slice.entry = instructions.front().address;
    slice.range_end = instructions.back().address + static_cast<std::uint32_t>(instructions.back().bytes.size());
    BasicBlock block{slice.entry, slice.range_end, {}};
    for (const auto& item : instructions) block.instruction_addresses.push_back(item.address);
    slice.instructions = std::move(instructions);
    slice.basic_blocks.push_back(std::move(block));
    return slice;
}

DecodedSlice merge_case(bool identical) {
    DecodedSlice slice;
    slice.entry = 0x1000;
    slice.range_end = 0x1024;
    slice.instructions = {
        instruction(0x1000, 0x6600, {0x66, 0x00, 0x00, 0x0C}, "bcc"),
        instruction(0x1004, 0x207C, {0x20, 0x7C, 0x00, 0x00, 0x11, 0x11}),
        instruction(0x100A, 0x6000, {0x60, 0x00, 0x00, 0x10}, "bra"),
        instruction(0x1010, 0x207C, {0x20, 0x7C, 0x00, 0x00, static_cast<std::uint8_t>(identical ? 0x11 : 0x22), 0x11}),
        instruction(0x1016, 0x6000, {0x60, 0x00, 0x00, 0x04}, "bra"),
        instruction(0x101A, 0x4A28, {0x4A, 0x28, 0x00, 0x01}),
    };
    slice.basic_blocks = {{0x1000, 0x1004, {0x1000}}, {0x1004, 0x1010, {0x1004, 0x100A}},
                          {0x1010, 0x101A, {0x1010, 0x1016}}, {0x101A, 0x101E, {0x101A}}};
    slice.control_flow = {{0x1000, 0x1010, FlowKind::direct_branch},
                          {0x100A, 0x101A, FlowKind::direct_branch},
                          {0x1016, 0x101A, FlowKind::direct_branch}};
    return slice;
}

void test_backward_definition() {
    const auto slice = linear({instruction(0x1000, 0x207C, {0x20, 0x7C, 0x00, 0x00, 0x12, 0x34}),
                               instruction(0x1006, 0x4A28, {0x4A, 0x28, 0x00, 0x01})});
    const auto result = analyze_bounded_backward_register(slice, 0x1006, 0);
    assert(result.value == 0x1234);
    assert(result.reason == ClosureReason::other);
    assert(result.provenance.size() == 1);
}

void test_call_and_redefinition() {
    auto call = instruction(0x1000, 0x6100, {0x61, 0x00, 0x00, 0x04}, "bsr");
    call.flow = FlowKind::direct_call;
    const auto clobber = analyze_bounded_backward_register(linear({call,
        instruction(0x1004, 0x4A28, {0x4A, 0x28, 0x00, 0x01})}), 0x1004, 0);
    assert(clobber.reason == ClosureReason::call_clobber);
    const auto resolved = analyze_bounded_backward_register(linear({call,
        instruction(0x1004, 0x207C, {0x20, 0x7C, 0x00, 0x00, 0x34, 0x56}),
        instruction(0x100A, 0x4A28, {0x4A, 0x28, 0x00, 0x01})}), 0x100A, 0);
    assert(resolved.value == 0x3456);
}

void test_merge_and_boundaries() {
    const auto same = analyze_bounded_backward_register(merge_case(true), 0x101A, 0);
    assert(same.value == 0x1111 && same.provenance.size() == 2);
    assert(analyze_bounded_backward_register(merge_case(false), 0x101A, 0).reason == ClosureReason::conflicting_cfg_merge);
    const auto unknown = analyze_bounded_backward_register(linear({instruction(0x1000, 0x4A28, {0x4A, 0x28, 0x00, 0x01})}), 0x1000, 0);
    assert(unknown.reason == ClosureReason::entry_state_unknown);
    const auto unknown_stack = analyze_bounded_backward_register(linear({instruction(0x1000, 0x205F, {0x20, 0x5F}, "movea"),
        instruction(0x1002, 0x4A28, {0x4A, 0x28, 0x00, 0x01})}), 0x1002, 0);
    assert(unknown_stack.reason == ClosureReason::entry_state_unknown && !unknown_stack.value &&
           !unknown_stack.definitions.empty() && unknown_stack.definitions.front().supported);
    DecodedSlice bounded = linear({instruction(0x1000, 0x4A28, {0x4A, 0x28, 0x00, 0x01})});
    bounded.basic_blocks.push_back({0x0FF0, 0x0FF6, {0x0FF0}});
    bounded.instructions.push_back(instruction(0x0FF0, 0x207C, {0x20, 0x7C, 0x00, 0x00, 0x99, 0x99}));
    const auto no_escape = analyze_bounded_backward_register(bounded, 0x1000, 0);
    assert(!no_escape.value && no_escape.reason == ClosureReason::entry_state_unknown);
}

void test_bounded_stack_provenance() {
    const auto pushed_immediate = analyze_bounded_backward_register(linear({
        instruction(0x1000, 0x2E7C, {0x2E, 0x7C, 0x00, 0x00, 0x10, 0x00}),
        instruction(0x1006, 0x2F3C, {0x2F, 0x3C, 0x00, 0x00, 0x12, 0x34}),
        instruction(0x100C, 0x205F, {0x20, 0x5F}, "movea"),
        instruction(0x100E, 0x4A28, {0x4A, 0x28, 0x00, 0x01})}), 0x100E, 0);
    assert(pushed_immediate.value == 0x1234 && pushed_immediate.a7_before == 0x0FFC && pushed_immediate.a7_increment_bytes == 4 &&
           pushed_immediate.stack_status == "proven_concrete_stack_value" && pushed_immediate.stack_provenance.size() == 2);

    const auto pushed_pea = analyze_bounded_backward_register(linear({
        instruction(0x1000, 0x2E7C, {0x2E, 0x7C, 0x00, 0x00, 0x10, 0x00}),
        instruction(0x1006, 0x4879, {0x48, 0x79, 0x00, 0x12, 0x34, 0x56}, "pea"),
        instruction(0x100C, 0x205F, {0x20, 0x5F}, "movea"),
        instruction(0x100E, 0x4A28, {0x4A, 0x28, 0x00, 0x01})}), 0x100E, 0);
    assert(pushed_pea.value == 0x123456 && pushed_pea.a7_before == 0x0FFC && pushed_pea.stack_status == "proven_concrete_stack_value");

    const auto pushed_register = analyze_bounded_backward_register(linear({
        instruction(0x1000, 0x2E7C, {0x2E, 0x7C, 0x00, 0x00, 0x10, 0x00}),
        instruction(0x1006, 0x227C, {0x22, 0x7C, 0x00, 0x00, 0x00, 0x42}),
        instruction(0x100C, 0x2F09, {0x2F, 0x09}, "move"),
        instruction(0x100E, 0x205F, {0x20, 0x5F}, "movea"),
        instruction(0x1010, 0x4A28, {0x4A, 0x28, 0x00, 0x01})}), 0x1010, 0);
    assert(pushed_register.value == 0x42 && pushed_register.a7_before == 0x0FFC && pushed_register.stack_status == "proven_concrete_stack_value");

    auto call = instruction(0x1000, 0x6100, {0x61, 0x00, 0x00, 0x04}, "bsr");
    call.flow = FlowKind::direct_call;
    const auto call_stack = analyze_bounded_backward_register(linear({call,
        instruction(0x1004, 0x205F, {0x20, 0x5F}, "movea"),
        instruction(0x1006, 0x4A28, {0x4A, 0x28, 0x00, 0x01})}), 0x1006, 0);
    assert(!call_stack.value && call_stack.reason == ClosureReason::other &&
           call_stack.stack_status == "stack_value_unknown_call_boundary");

    const auto conflicting = [] {
        DecodedSlice slice;
        slice.entry = 0x1000;
        slice.range_end = 0x1026;
        slice.instructions = {
            instruction(0x1000, 0x2E7C, {0x2E, 0x7C, 0x00, 0x01, 0x00, 0x00}),
            instruction(0x1006, 0x6600, {0x66, 0x00, 0x00, 0x10}, "bcc"),
            instruction(0x100A, 0x2F3C, {0x2F, 0x3C, 0x00, 0x00, 0x11, 0x11}),
            instruction(0x1010, 0x6000, {0x60, 0x00, 0x00, 0x0E}, "bra"),
            instruction(0x1016, 0x2F3C, {0x2F, 0x3C, 0x00, 0x00, 0x22, 0x22}),
            instruction(0x101C, 0x6000, {0x60, 0x00, 0x00, 0x02}, "bra"),
            instruction(0x1020, 0x205F, {0x20, 0x5F}, "movea"),
            instruction(0x1022, 0x4A28, {0x4A, 0x28, 0x00, 0x01}),
        };
        slice.basic_blocks = {{0x1000, 0x1006, {0x1000}}, {0x1006, 0x100A, {0x1006}},
                              {0x100A, 0x1016, {0x100A, 0x1010}}, {0x1016, 0x1020, {0x1016, 0x101C}},
                              {0x1020, 0x1026, {0x1020, 0x1022}}};
        slice.control_flow = {{0x1006, 0x1016, FlowKind::direct_branch},
                              {0x1010, 0x1020, FlowKind::direct_branch}, {0x101C, 0x1020, FlowKind::direct_branch}};
        return slice;
    }();
    const auto conflict = analyze_bounded_backward_register(conflicting, 0x1022, 0);
    assert(!conflict.value && conflict.reason == ClosureReason::conflicting_cfg_merge &&
           conflict.stack_status == "conflicting_stack_merge");

    const auto unrelated_postincrement = analyze_bounded_backward_register(linear({
        instruction(0x1000, 0x201F, {0x20, 0x1F}),
        instruction(0x1002, 0x4A28, {0x4A, 0x28, 0x00, 0x01})}), 0x1002, 0);
    assert(!unrelated_postincrement.value && unrelated_postincrement.reason == ClosureReason::entry_state_unknown);
}

} // namespace

int main() {
    test_backward_definition();
    test_call_and_redefinition();
    test_merge_and_boundaries();
    test_bounded_stack_provenance();
    ReachableClosureReport report;
    report.reason_counts = {{"call_clobber", 1}};
    assert(reachable_closure_to_json(report).find("oasis.m68k.re-reachable-closure.v1") != std::string::npos);
    assert(reachable_closure_to_json(report) == reachable_closure_to_json(report));
    assert(closure_reason_name(ClosureReason::entry_state_unknown) == "entry_state_unknown");
    return 0;
}
