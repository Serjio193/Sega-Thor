#include "tools/re_callee_effect.hpp"

#include <cassert>
#include <cstdint>
#include <initializer_list>

namespace {

using namespace oasis::tools;

DecodedInstruction instruction(std::uint32_t address, std::uint16_t opcode,
                               std::initializer_list<std::uint8_t> bytes,
                               const char* mnemonic, FlowKind flow = FlowKind::none) {
    return {.address = address, .opcode = opcode, .bytes = bytes, .mnemonic = mnemonic,
            .supported = true, .flow = flow};
}

DecodedInstruction known_lea(std::uint32_t address, std::uint32_t value) {
    auto result = instruction(address, 0x41F9, {0x41, 0xF9, 0, 0, 0, 0}, "lea");
    result.bytes[2] = static_cast<std::uint8_t>(value >> 24U);
    result.bytes[3] = static_cast<std::uint8_t>(value >> 16U);
    result.bytes[4] = static_cast<std::uint8_t>(value >> 8U);
    result.bytes[5] = static_cast<std::uint8_t>(value);
    result.memory_references.push_back({value, 4, MemoryKind::rom, MemoryAccess::address});
    result.addressing_modes = {"absolute_long"};
    return result;
}

DecodedSlice callee(std::vector<DecodedInstruction> instructions,
                    std::vector<BasicBlock> blocks,
                    std::vector<ControlFlowEdge> edges = {}) {
    DecodedSlice result;
    result.entry = blocks.front().start;
    result.range_end = blocks.back().end;
    result.instructions = std::move(instructions);
    result.basic_blocks = std::move(blocks);
    result.control_flow = std::move(edges);
    return result;
}

DecodedSlice call_site(std::uint32_t target) {
    DecodedSlice result;
    result.entry = 0x9000;
    result.range_end = 0x9004;
    result.instructions.push_back(instruction(0x9000, 0x6100, {0x61, 0, 0, 0}, "bsr", FlowKind::direct_call));
    result.instructions.back().direct_target = target;
    result.basic_blocks.push_back({0x9000, 0x9004, {0x9000}});
    result.control_flow.push_back({0x9000, target, FlowKind::direct_call});
    return result;
}

CalleeEffectReport audit(DecodedSlice body) {
    return analyze_callee_effect(call_site(body.entry), body);
}

std::string effect(const CalleeEffectReport& report, std::uint8_t index) {
    return report.register_effects[index].effect;
}

void test_preserved_and_clobbered() {
    const auto preserved = audit(callee({instruction(0x8000, 0x4E71, {0x4E, 0x71}, "nop"),
                                         instruction(0x8002, 0x4E75, {0x4E, 0x75}, "rts", FlowKind::return_instruction)},
                                        {{0x8000, 0x8004, {0x8000, 0x8002}}}));
    assert(preserved.boundary_proven && preserved.return_sites.size() == 1U);
    assert(effect(preserved, 0) == "not_touched" && effect(preserved, 6) == "not_touched");
    assert(effect(preserved, 7) == "preserved" && preserved.stack_effect.status == "CONFIRMED");

    const auto clobbered = audit(callee({known_lea(0x8100, 0x00001234),
                                         instruction(0x8106, 0x4E75, {0x4E, 0x75}, "rts", FlowKind::return_instruction)},
                                        {{0x8100, 0x8108, {0x8100, 0x8106}}}));
    assert(effect(clobbered, 0) == "overwritten_known" && clobbered.register_effects[0].known_value == 0x1234U);
}

void test_path_dependent_and_multiple_returns() {
    const auto report = audit(callee({
        instruction(0x8200, 0x6604, {0x66, 0x04}, "bcc", FlowKind::direct_branch),
        instruction(0x8202, 0x4E75, {0x4E, 0x75}, "rts", FlowKind::return_instruction),
        known_lea(0x8206, 0x00005678),
        instruction(0x820C, 0x4E75, {0x4E, 0x75}, "rts", FlowKind::return_instruction)},
        {{0x8200, 0x8202, {0x8200}}, {0x8202, 0x8204, {0x8202}},
         {0x8206, 0x820C, {0x8206, 0x820C}}},
        {{0x8200, 0x8206, FlowKind::direct_branch}}));
    assert(report.return_sites.size() == 2U && report.reachable_blocks.size() == 3U);
    assert(effect(report, 0) == "path_dependent" && report.stack_effect.status == "CONFIRMED");
}

void test_stack_rules() {
    const auto extra_balanced = audit(callee({
        instruction(0x8300, 0x4879, {0x48, 0x79}, "pea"),
        instruction(0x8302, 0x205F, {0x20, 0x5F}, "movea"),
        instruction(0x8304, 0x4E75, {0x4E, 0x75}, "rts", FlowKind::return_instruction)},
        {{0x8300, 0x8306, {0x8300, 0x8302, 0x8304}}}));
    assert(extra_balanced.stack_effect.status == "CONFIRMED");

    const auto unbalanced = audit(callee({instruction(0x8400, 0x4879, {0x48, 0x79}, "pea"),
                                           instruction(0x8402, 0x4E75, {0x4E, 0x75}, "rts", FlowKind::return_instruction)},
                                          {{0x8400, 0x8404, {0x8400, 0x8402}}}));
    assert(unbalanced.stack_effect.status == "UNKNOWN" && effect(unbalanced, 7) == "overwritten_unknown");

    const auto return_address_pop = audit(callee({instruction(0x8500, 0x205F, {0x20, 0x5F}, "movea"),
                                                   instruction(0x8502, 0x4E75, {0x4E, 0x75}, "rts", FlowKind::return_instruction)},
                                                  {{0x8500, 0x8504, {0x8500, 0x8502}}}));
    assert(return_address_pop.stack_effect.status == "UNKNOWN" && effect(return_address_pop, 0) == "overwritten_unknown");

    const auto unrelated = audit(callee({instruction(0x8600, 0x201F, {0x20, 0x1F}, "move"),
                                         instruction(0x8602, 0x4E75, {0x4E, 0x75}, "rts", FlowKind::return_instruction)},
                                        {{0x8600, 0x8604, {0x8600, 0x8602}}}));
    assert(unrelated.stack_effect.status == "CONFIRMED" && effect(unrelated, 0) == "not_touched");

    auto call = instruction(0x8700, 0x6100, {0x61, 0, 0, 0}, "bsr", FlowKind::direct_call);
    call.direct_target = 0x8800;
    const auto call_boundary = audit(callee({call, instruction(0x8704, 0x4E75, {0x4E, 0x75}, "rts", FlowKind::return_instruction)},
                                             {{0x8700, 0x8706, {0x8700, 0x8704}}}));
    assert(call_boundary.stack_effect.status == "UNKNOWN" && effect(call_boundary, 7) == "overwritten_unknown");
}

void test_deterministic_format() {
    const auto report = audit(callee({instruction(0x8900, 0x4E75, {0x4E, 0x75}, "rts", FlowKind::return_instruction)},
                                     {{0x8900, 0x8902, {0x8900}}}));
    assert(callee_effect_to_json(report) == callee_effect_to_json(report));
    assert(callee_effect_to_json(report).find("oasis.m68k.re-callee-effect.v1") != std::string::npos);
    assert(callee_effect_to_text(report).find("register_effects") != std::string::npos);
}

} // namespace

int main() {
    test_preserved_and_clobbered();
    test_path_dependent_and_multiple_returns();
    test_stack_rules();
    test_deterministic_format();
    return 0;
}
