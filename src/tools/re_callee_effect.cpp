#include "tools/re_callee_effect.hpp"

#include "tools/re_slice_decoder.hpp"

#include <algorithm>
#include <array>
#include <map>
#include <set>
#include <tuple>

namespace oasis::tools {
namespace {

struct RegisterState {
    std::string effect{"not_touched"};
    std::optional<std::uint32_t> value;
    std::vector<std::uint32_t> evidence;
};

struct PathState {
    std::array<RegisterState, 8> registers;
    int stack_delta{};
    bool unknown_call{};
    bool returned{};
};

struct FlowContext {
    const DecodedSlice& slice;
    std::map<std::uint32_t, std::size_t> blocks;
    std::vector<bool> reachable;
};

const DecodedInstruction* instruction_at(const DecodedSlice& slice, std::uint32_t address) {
    const auto found = std::find_if(slice.instructions.begin(), slice.instructions.end(),
                                    [=](const auto& item) { return item.address == address; });
    return found == slice.instructions.end() ? nullptr : &*found;
}

bool is_in_slice(const DecodedSlice& slice, std::uint32_t address) {
    return std::any_of(slice.basic_blocks.begin(), slice.basic_blocks.end(),
                       [=](const auto& block) { return block.start == address; });
}

std::uint32_t block_start(const DecodedSlice& slice, std::uint32_t address) {
    for (const auto& block : slice.basic_blocks)
        if (std::find(block.instruction_addresses.begin(), block.instruction_addresses.end(), address) != block.instruction_addresses.end())
            return block.start;
    return address;
}

std::vector<std::uint32_t> successors(const DecodedSlice& slice, const BasicBlock& block) {
    if (block.instruction_addresses.empty()) return {};
    const auto* last = instruction_at(slice, block.instruction_addresses.back());
    if (!last) return {};
    std::vector<std::uint32_t> result;
    for (const auto& edge : slice.control_flow)
        if (edge.source == last->address && is_in_slice(slice, edge.target)) result.push_back(edge.target);
    const bool fallthrough = last->flow == FlowKind::direct_call ||
        (last->flow == FlowKind::direct_branch && last->mnemonic != "bra") || last->flow == FlowKind::none;
    if (fallthrough && is_in_slice(slice, block.end)) result.push_back(block.end);
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

FlowContext make_context(const DecodedSlice& slice) {
    FlowContext context{slice, {}, std::vector<bool>(slice.basic_blocks.size(), false)};
    for (std::size_t i = 0; i < slice.basic_blocks.size(); ++i) context.blocks[slice.basic_blocks[i].start] = i;
    const auto entry = context.blocks.find(slice.entry);
    if (entry == context.blocks.end()) return context;
    std::vector<std::size_t> pending{entry->second};
    context.reachable[entry->second] = true;
    while (!pending.empty()) {
        const auto current = pending.back();
        pending.pop_back();
        for (const auto target : successors(slice, slice.basic_blocks[current])) {
            const auto found = context.blocks.find(target);
            if (found != context.blocks.end() && !context.reachable[found->second]) {
                context.reachable[found->second] = true;
                pending.push_back(found->second);
            }
        }
    }
    return context;
}

void mark_unknown(RegisterState& state, std::uint32_t address) {
    state.effect = "overwritten_unknown";
    state.value.reset();
    state.evidence.push_back(address);
}

void apply_instruction(PathState& state, const DecodedInstruction& instruction) {
    const auto opcode = instruction.opcode;
    const auto source_mode = static_cast<std::uint8_t>((opcode >> 3U) & 7U);
    const auto destination_mode = static_cast<std::uint8_t>((opcode >> 6U) & 7U);
    const auto destination = static_cast<std::uint8_t>((opcode >> 9U) & 7U);
    if (instruction.mnemonic == "lea") {
        auto& target = state.registers[destination];
        if (instruction.unresolved_memory_references.empty() && !instruction.memory_references.empty()) {
            target.effect = "overwritten_known";
            target.value = instruction.memory_references.front().address;
        } else mark_unknown(target, instruction.address);
        target.evidence.push_back(instruction.address);
    } else if ((opcode >> 12U) >= 1U && (opcode >> 12U) <= 3U && destination_mode == 1U) {
        auto& target = state.registers[destination];
        if (source_mode == 7U && (opcode & 7U) == 4U && !instruction.immediate_constants.empty()) {
            target.effect = "overwritten_known";
            target.value = instruction.immediate_constants.front().value;
            target.evidence.push_back(instruction.address);
        } else mark_unknown(target, instruction.address);
    } else if (instruction.mnemonic == "scc" && source_mode == 3U) {
        mark_unknown(state.registers[opcode & 7U], instruction.address);
    } else if (instruction.mnemonic == "link") {
        mark_unknown(state.registers[6], instruction.address);
        mark_unknown(state.registers[7], instruction.address);
    } else if (instruction.mnemonic == "unlk") {
        mark_unknown(state.registers[opcode & 7U], instruction.address);
        mark_unknown(state.registers[7], instruction.address);
    }
    if (instruction.flow == FlowKind::direct_call) {
        state.unknown_call = true;
        for (auto& register_state : state.registers) mark_unknown(register_state, instruction.address);
    }
    if (opcode == 0x2F3CU || instruction.mnemonic == "pea") state.stack_delta -= 4;
    if ((opcode & 0xF1FFU) == 0x205FU && destination_mode == 1U && source_mode == 3U && (opcode & 7U) == 7U) {
        mark_unknown(state.registers[destination], instruction.address);
        state.stack_delta += 4;
    }
}

void collect_paths(const FlowContext& context, std::size_t block_index, PathState state,
                   std::set<std::size_t> active, std::vector<PathState>& paths) {
    if (!active.insert(block_index).second) {
        state.returned = false;
        paths.push_back(std::move(state));
        return;
    }
    const auto& block = context.slice.basic_blocks[block_index];
    for (const auto address : block.instruction_addresses) {
        const auto* instruction = instruction_at(context.slice, address);
        if (!instruction) continue;
        apply_instruction(state, *instruction);
        if (instruction->flow == FlowKind::return_instruction || instruction->flow == FlowKind::unsupported) {
            state.returned = instruction->flow == FlowKind::return_instruction;
            paths.push_back(std::move(state));
            return;
        }
    }
    const auto next = successors(context.slice, block);
    if (next.empty()) {
        state.returned = false;
        paths.push_back(std::move(state));
        return;
    }
    for (const auto target : next) {
        const auto found = context.blocks.find(target);
        if (found != context.blocks.end() && context.reachable[found->second])
            collect_paths(context, found->second, state, active, paths);
    }
}

bool same_state(const RegisterState& left, const RegisterState& right) {
    return left.effect == right.effect && left.value == right.value;
}

std::string memory_class(MemoryKind kind) { return memory_kind_name(kind); }

void add_memory(std::vector<CalleeMemoryEvidence>& output, const DecodedInstruction& instruction,
                const MemoryReference& reference, std::uint32_t containing_block) {
    output.push_back({instruction.address, containing_block, reference.address, reference.width_bytes,
                      memory_access_name(reference.access), memory_class(reference.kind), "confirmed decoder reference"});
}

void add_unresolved(std::vector<CalleeMemoryEvidence>& output, const DecodedInstruction& instruction,
                    const UnresolvedMemoryReference& reference, std::uint32_t containing_block) {
    output.push_back({instruction.address, containing_block, std::nullopt, 0, "unknown", "unresolved", "mode " +
                      std::to_string(reference.mode) + "/register " + std::to_string(reference.register_index) + ": " + reference.reason});
}

} // namespace

CalleeEffectReport analyze_callee_effect(const DecodedSlice& call_site_slice, const DecodedSlice& callee_slice) {
    CalleeEffectReport report{.requested_entry = call_site_slice.entry, .call_site = call_site_slice.entry,
                              .entry = callee_slice.entry, .bounded_start = callee_slice.entry,
                              .bounded_end = callee_slice.range_end, .boundary_proven = false,
                              .boundary_status = "bounded_code"};
    for (const auto& edge : call_site_slice.control_flow)
        if (edge.source == call_site_slice.entry && edge.kind == FlowKind::direct_call) report.entry = edge.target;
    const auto context = make_context(callee_slice);
    for (std::size_t i = 0; i < callee_slice.basic_blocks.size(); ++i)
        if (context.reachable[i]) report.reachable_blocks.push_back(callee_slice.basic_blocks[i].start);
    for (const auto& instruction : callee_slice.instructions) {
        if (instruction.flow == FlowKind::return_instruction) report.return_sites.push_back(instruction.address);
        if (instruction.flow == FlowKind::direct_call && instruction.direct_target) report.direct_callees.push_back(*instruction.direct_target);
        if (instruction.flow == FlowKind::indirect_call || instruction.flow == FlowKind::indirect_jump)
            report.indirect_flow.push_back(instruction.address);
        if (!instruction.supported) report.unsupported_instructions.push_back(instruction.address);
        const auto containing_block = block_start(callee_slice, instruction.address);
        for (const auto& reference : instruction.memory_references) add_memory(report.memory_references, instruction, reference, containing_block);
        for (const auto& reference : instruction.unresolved_memory_references) add_unresolved(report.unresolved_memory_references, instruction, reference, containing_block);
    }
    report.control_flow_edges = callee_slice.control_flow;
    std::sort(report.control_flow_edges.begin(), report.control_flow_edges.end(), [](const auto& left, const auto& right) {
        return std::tie(left.source, left.target, left.kind) < std::tie(right.source, right.target, right.kind);
    });
    if (!report.return_sites.empty()) {
        const auto* last = instruction_at(callee_slice, report.return_sites.front());
        report.bounded_end = last ? last->address + static_cast<std::uint32_t>(last->bytes.size()) : report.bounded_end;
        report.boundary_proven = report.return_sites.size() == 1U && report.unsupported_instructions.empty();
        report.boundary_status = report.boundary_proven ? "return_terminated_bounded_code" : "bounded_code_with_multiple_or_unsupported_returns";
    }
    std::sort(report.reachable_blocks.begin(), report.reachable_blocks.end());
    std::sort(report.return_sites.begin(), report.return_sites.end());
    std::sort(report.direct_callees.begin(), report.direct_callees.end());
    report.direct_callees.erase(std::unique(report.direct_callees.begin(), report.direct_callees.end()), report.direct_callees.end());
    std::vector<PathState> paths;
    const auto entry = context.blocks.find(callee_slice.entry);
    if (entry != context.blocks.end() && context.reachable[entry->second]) collect_paths(context, entry->second, {}, {}, paths);
    const bool all_paths_returned = !paths.empty() && std::all_of(paths.begin(), paths.end(), [](const auto& path) { return path.returned; });
    for (std::uint8_t register_index = 0; register_index < 8U; ++register_index) {
        CalleeRegisterEffect effect{.register_index = register_index};
        if (!all_paths_returned) effect.effect = "overwritten_unknown";
        else {
            effect.effect = paths.front().registers[register_index].effect;
            effect.known_value = paths.front().registers[register_index].value;
            bool same = true;
            for (const auto& path : paths) {
                same = same && same_state(paths.front().registers[register_index], path.registers[register_index]);
                effect.evidence_instructions.insert(effect.evidence_instructions.end(), path.registers[register_index].evidence.begin(), path.registers[register_index].evidence.end());
            }
            if (!same) {
                effect.effect = "path_dependent";
                effect.known_value.reset();
            }
        }
        std::sort(effect.evidence_instructions.begin(), effect.evidence_instructions.end());
        effect.evidence_instructions.erase(std::unique(effect.evidence_instructions.begin(), effect.evidence_instructions.end()), effect.evidence_instructions.end());
        if (register_index == 7U && !paths.empty()) {
            const bool balanced = std::all_of(paths.begin(), paths.end(), [](const auto& path) {
                return path.returned && !path.unknown_call && path.stack_delta == 0;
            });
            const bool same_delta = std::all_of(paths.begin(), paths.end(), [&](const auto& path) {
                return path.stack_delta == paths.front().stack_delta;
            });
            if (balanced) effect.effect = "preserved";
            else if (!same_delta) effect.effect = "path_dependent";
            else effect.effect = "overwritten_unknown";
            effect.known_value.reset();
        }
        report.register_effects.push_back(std::move(effect));
    }
    const bool balanced = !paths.empty() && std::all_of(paths.begin(), paths.end(), [](const auto& path) {
        return path.returned && !path.unknown_call && path.stack_delta == 0;
    });
    std::string internal_operations = "unknown";
    if (!paths.empty()) {
        const bool same_delta = std::all_of(paths.begin(), paths.end(), [&](const auto& path) {
            return path.stack_delta == paths.front().stack_delta;
        });
        if (same_delta) internal_operations = "explicit callee delta is " + std::to_string(paths.front().stack_delta) + " bytes on every analyzed path";
        else internal_operations = "explicit callee delta is path-dependent";
    }
    report.stack_effect = {"caller_pre_bsr", "pushes return address at caller_pre_bsr-4",
                           internal_operations,
                           "RTS pops the BSR return address", balanced ? "caller_pre_bsr" : "unknown",
                           balanced ? "CONFIRMED" : "UNKNOWN"};
    report.provenance = {"call-site 0x60BCC bytes 61 00 F8 EE decode as BSR.W -> 0x604BC",
                         "callee bounded from 0x604BC through RTS at 0x604E4",
                         "0x60BD0 is outside callee and executes after RTS; it reads memory at caller_pre_bsr A7"};
    report.target_rechecks = {
        {0x60BFA, "unsupported_transfer", "0x60BCC BSR -> 0x604BC .. RTS 0x604E4 -> 0x60BD0 MOVEA.L (A7)+,A0", "caller_pre_bsr (relative; not concrete)", "unknown longword at caller_pre_bsr", std::nullopt, "unknown", "unresolved_stack_value"},
        {0x60C08, "unsupported_transfer", "0x60BCC BSR -> 0x604BC .. RTS 0x604E4 -> 0x60BD0 MOVEA.L (A7)+,A0", "caller_pre_bsr (relative; not concrete)", "unknown longword at caller_pre_bsr", std::nullopt, "unknown", "unresolved_stack_value"},
    };
    return report;
}

CalleeEffectReport audit_callee_effect(std::span<const std::uint8_t> retail_rom) {
    const auto call_site = decode_m68k_slice(retail_rom, {.entry = 0x60BCCU, .byte_budget = 4U});
    std::uint32_t target = 0x60BCCU;
    for (const auto& edge : call_site.control_flow)
        if (edge.source == 0x60BCCU && edge.kind == FlowKind::direct_call) target = edge.target;
    const auto callee = decode_m68k_slice(retail_rom, {.entry = target, .byte_budget = 0x400U});
    return analyze_callee_effect(call_site, callee);
}

} // namespace oasis::tools
