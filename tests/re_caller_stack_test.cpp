#include "tools/re_caller_stack.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <initializer_list>

namespace {

using namespace oasis::tools;

DecodedInstruction instruction(std::uint32_t address, std::uint16_t opcode,
                               std::initializer_list<std::uint8_t> bytes,
                               const char* mnemonic, FlowKind flow = FlowKind::none,
                               std::optional<std::uint32_t> target = std::nullopt) {
    auto result = DecodedInstruction{.address = address, .opcode = opcode, .bytes = bytes,
                                     .mnemonic = mnemonic, .supported = true, .flow = flow};
    result.direct_target = target;
    return result;
}

DecodedInstruction push_immediate(std::uint32_t address, std::uint32_t value) {
    auto result = instruction(address, 0x2F3C, {0x2F, 0x3C, 0, 0, 0, 0}, "move");
    result.immediate_constants.push_back({value, 4});
    return result;
}

DecodedInstruction movea_pop(std::uint32_t address) {
    return instruction(address, 0x205F, {0x20, 0x5F}, "movea");
}

DecodedInstruction pea_known(std::uint32_t address, std::uint32_t value) {
    auto result = instruction(address, 0x4879, {0x48, 0x79}, "pea");
    result.memory_references.push_back({value, 4, MemoryKind::rom, MemoryAccess::address});
    return result;
}

DecodedInstruction call(std::uint32_t address, std::uint32_t target = 0x604BCU) {
    return instruction(address, 0x6100, {0x61, 0, 0, 0}, "bsr", FlowKind::direct_call, target);
}

DecodedSlice caller(std::vector<DecodedInstruction> instructions,
                    std::vector<BasicBlock> blocks, std::vector<ControlFlowEdge> edges = {}) {
    return {.entry = blocks.front().start, .range_end = 0x60BD0U,
            .instructions = std::move(instructions), .basic_blocks = std::move(blocks),
            .control_flow = std::move(edges)};
}

CalleeEffectReport preserved_callee() {
    CalleeEffectReport result;
    result.entry = 0x604BCU;
    result.stack_effect.status = "CONFIRMED";
    return result;
}

DecodedSlice straight(std::uint32_t value) {
    return caller({push_immediate(0x60BB0U, value), call(0x60BCCU)},
                  {{0x60BB0U, 0x60BCCU, {0x60BB0U}}, {0x60BCCU, 0x60BD0U, {0x60BCCU}}});
}

void test_pre_call_value_and_return_address() {
    const auto report = analyze_caller_stack(straight(0x1234U), preserved_callee());
    assert(report.relevant_path_count == 1U && report.value_at_pre_call_sp);
    assert(report.value_at_pre_call_sp->expression == "0x00001234");
    assert(report.value_kind == "immediate_constant" && report.merge_status == "SINGLE_PATH_PROVEN");
    assert(report.stack_event_count == 2U && report.stack_events[0].event == "MOVE.L #imm,-(A7)");
    const auto call_event = std::find_if(report.stack_events.begin(), report.stack_events.end(),
                                         [](const auto& item) { return item.event == "BSR + proven callee + RTS"; });
    assert(call_event != report.stack_events.end());
    assert(call_event->value.find("60BD0") != std::string::npos);
}

void test_pea_and_movea_postincrement() {
    const auto pea = caller({pea_known(0x60BB0U, 0x000638D4U), call(0x60BCCU)},
        {{0x60BB0U, 0x60BCCU, {0x60BB0U}}, {0x60BCCU, 0x60BD0U, {0x60BCCU}}});
    const auto pea_report = analyze_caller_stack(pea, preserved_callee());
    assert(pea_report.value_at_pre_call_sp && pea_report.value_at_pre_call_sp->expression == "0x000638D4");
    assert(pea_report.value_kind == "ROM_address");

    const auto pop_then_push = caller({push_immediate(0x60BB0U, 0x1234U), movea_pop(0x60BB6U),
                                       push_immediate(0x60BB8U, 0x5678U), call(0x60BCCU)},
        {{0x60BB0U, 0x60BCCU, {0x60BB0U, 0x60BB6U, 0x60BB8U}},
         {0x60BCCU, 0x60BD0U, {0x60BCCU}}});
    const auto pop_report = analyze_caller_stack(pop_then_push, preserved_callee());
    assert(pop_report.value_at_pre_call_sp && pop_report.value_at_pre_call_sp->expression == "0x00005678");
    const auto pop_event = std::find_if(pop_report.stack_events.begin(), pop_report.stack_events.end(),
                                        [](const auto& item) { return item.instruction_address == 0x60BB6U; });
    assert(pop_event != pop_report.stack_events.end() && pop_event->sp_before == "S-4" && pop_event->sp_after == "S");
}

void test_unknown_stack_value_and_limited_real_pushes() {
    auto unknown_push = instruction(0x60BB0U, 0x2F3C, {0x2F, 0x3C}, "move");
    const auto unknown = caller({unknown_push, call(0x60BCCU)},
        {{0x60BB0U, 0x60BCCU, {0x60BB0U}}, {0x60BCCU, 0x60BD0U, {0x60BCCU}}});
    const auto unknown_report = analyze_caller_stack(unknown, preserved_callee());
    assert(!unknown_report.value_at_pre_call_sp && unknown_report.merge_status == "UNKNOWN_VALUE");

    auto status = instruction(0x60BB0U, 0x40E7, {0x40, 0xE7}, "move_status");
    auto movem = instruction(0x60BB2U, 0x48E7, {0x48, 0xE7, 0x00, 0x03}, "movem");
    const auto real_pushes = caller({status, movem, push_immediate(0x60BB6U, 0xCAFEU), call(0x60BCCU)},
        {{0x60BB0U, 0x60BCCU, {0x60BB0U, 0x60BB2U, 0x60BB6U}},
         {0x60BCCU, 0x60BD0U, {0x60BCCU}}});
    const auto real_push_report = analyze_caller_stack(real_pushes, preserved_callee());
    assert(real_push_report.value_at_pre_call_sp && real_push_report.symbolic_pre_call_sp == "P=S-14");
}

DecodedSlice agreeing_paths(bool conflict) {
    return caller({
        instruction(0x60BB0U, 0x6604, {0x66, 0x04}, "bcc", FlowKind::direct_branch, 0x60BB6U),
        push_immediate(0x60BB2U, 0x1111U),
        instruction(0x60BB8U, 0x6002, {0x60, 0x02}, "bra", FlowKind::direct_branch, 0x60BBCU),
        push_immediate(0x60BB6U, conflict ? 0x2222U : 0x1111U),
        instruction(0x60BBCU, 0x4E71, {0x4E, 0x71}, "nop"),
        call(0x60BCCU)},
        {{0x60BB0U, 0x60BB2U, {0x60BB0U}}, {0x60BB2U, 0x60BB8U, {0x60BB2U}},
         {0x60BB6U, 0x60BB8U, {0x60BB6U}}, {0x60BB8U, 0x60BBBU, {0x60BB8U}},
         {0x60BBCU, 0x60BCCU, {0x60BBCU}}, {0x60BCCU, 0x60BD0U, {0x60BCCU}}},
        {{0x60BB0U, 0x60BB6U, FlowKind::direct_branch},
         {0x60BB8U, 0x60BBCU, FlowKind::direct_branch}});
}

void test_cfg_merge() {
    const auto agreeing = analyze_caller_stack(agreeing_paths(false), preserved_callee());
    assert(agreeing.relevant_path_count == 2U && agreeing.merge_status == "AGREEING_PATHS");
    assert(agreeing.value_at_pre_call_sp && agreeing.stack_merge_conflicts == 0U);
    const auto conflict = analyze_caller_stack(agreeing_paths(true), preserved_callee());
    assert(conflict.relevant_path_count == 2U && conflict.merge_status == "CONFLICTING_VALUES");
    assert(!conflict.value_at_pre_call_sp && conflict.stack_merge_conflicts == 1U);
}

void test_unknown_call_and_bounded_escape() {
    const auto unknown_call = caller({call(0x60BB0U, 0x1234U), call(0x60BCCU)},
        {{0x60BB0U, 0x60BCCU, {0x60BB0U}}, {0x60BCCU, 0x60BD0U, {0x60BCCU}}});
    const auto blocked = analyze_caller_stack(unknown_call, preserved_callee());
    assert(blocked.relevant_path_count == 1U && blocked.unknown_call_effects == 1U);
    assert(!blocked.value_at_pre_call_sp && std::any_of(blocked.blockers.begin(), blocked.blockers.end(),
                                                        [](const auto& item) { return item.find("0x00060BB0") != std::string::npos; }));

    DecodedSlice escape;
    escape.entry = 0x60BB0U;
    escape.range_end = 0x60BB2U;
    escape.instructions.push_back(instruction(0x60BB0U, 0x6000, {0x60, 0}, "bra", FlowKind::direct_branch, 0x7000U));
    escape.basic_blocks.push_back({0x60BB0U, 0x60BB2U, {0x60BB0U}});
    const auto escaped = analyze_caller_stack(escape, preserved_callee());
    assert(escaped.relevant_path_count == 0U && escaped.merge_status == "NO_REACHABLE_PATH");
}

void test_format_is_deterministic() {
    const auto report = analyze_caller_stack(straight(0x1234U), preserved_callee());
    assert(caller_stack_to_json(report) == caller_stack_to_json(report));
    assert(caller_stack_to_json(report).find("oasis.m68k.re-caller-stack.v1") != std::string::npos);
    assert(caller_stack_to_text(report).find("memory[P]") == std::string::npos);
}

} // namespace

int main() {
    test_pre_call_value_and_return_address();
    test_pea_and_movea_postincrement();
    test_unknown_stack_value_and_limited_real_pushes();
    test_cfg_merge();
    test_unknown_call_and_bounded_escape();
    test_format_is_deterministic();
    return 0;
}
