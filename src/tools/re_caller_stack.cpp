#include "tools/re_caller_stack.hpp"

#include "tools/re_slice_decoder.hpp"

#include <algorithm>
#include <array>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <tuple>
#include <utility>

namespace oasis::tools {
namespace {

struct StackState {
    std::optional<int> sp_offset{0};
    std::map<int, CallerStackValue> slots;
    std::array<std::optional<CallerStackValue>, 8> address_registers;
    std::vector<CallerStackEvent> events;
    std::vector<std::uint32_t> block_starts;
    std::vector<std::string> blockers;
};

struct FlowContext {
    const DecodedSlice& slice;
    std::map<std::uint32_t, std::size_t> blocks;
    std::map<std::uint32_t, std::size_t> instructions;
    std::vector<std::vector<std::size_t>> predecessors;
    std::vector<bool> reachable;
    std::vector<int> distance_to_call;
};

std::string hex32(std::uint32_t value) {
    std::ostringstream out;
    out << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << value;
    return out.str();
}

std::string sp_text(const std::optional<int>& offset) {
    if (!offset) return "UNKNOWN";
    if (*offset == 0) return "S";
    return *offset > 0 ? "S+" + std::to_string(*offset) : "S" + std::to_string(*offset);
}

const DecodedInstruction* instruction_at(const DecodedSlice& slice, std::uint32_t address) {
    const auto found = std::find_if(slice.instructions.begin(), slice.instructions.end(),
                                    [=](const auto& item) { return item.address == address; });
    return found == slice.instructions.end() ? nullptr : &*found;
}

std::uint32_t block_start(const DecodedSlice& slice, std::uint32_t address) {
    for (const auto& block : slice.basic_blocks)
        if (std::find(block.instruction_addresses.begin(), block.instruction_addresses.end(), address) != block.instruction_addresses.end())
            return block.start;
    return address;
}

bool has_block(const DecodedSlice& slice, std::uint32_t address) {
    return std::any_of(slice.basic_blocks.begin(), slice.basic_blocks.end(),
                       [=](const auto& block) { return block.start == address; });
}

std::vector<std::uint32_t> successors(const DecodedSlice& slice, const BasicBlock& block) {
    if (block.instruction_addresses.empty()) return {};
    const auto* last = instruction_at(slice, block.instruction_addresses.back());
    if (!last) return {};
    std::vector<std::uint32_t> result;
    for (const auto& edge : slice.control_flow) {
        if (edge.source != last->address || !has_block(slice, edge.target)) continue;
        if (last->flow != FlowKind::direct_call) result.push_back(edge.target);
    }
    const bool fallthrough = last->flow == FlowKind::direct_call ||
        (last->flow == FlowKind::direct_branch && last->mnemonic != "bra") || last->flow == FlowKind::none;
    if (fallthrough && has_block(slice, block.end)) result.push_back(block.end);
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

FlowContext make_context(const DecodedSlice& slice) {
    FlowContext result{slice, {}, {}, std::vector<std::vector<std::size_t>>(slice.basic_blocks.size()),
                       std::vector<bool>(slice.basic_blocks.size(), false)};
    for (std::size_t index = 0; index < slice.basic_blocks.size(); ++index) {
        result.blocks[slice.basic_blocks[index].start] = index;
        for (const auto address : slice.basic_blocks[index].instruction_addresses) result.instructions[address] = index;
    }
    for (std::size_t index = 0; index < slice.basic_blocks.size(); ++index)
        for (const auto target : successors(slice, slice.basic_blocks[index])) {
            const auto found = result.blocks.find(target);
            if (found != result.blocks.end()) result.predecessors[found->second].push_back(index);
        }
    const auto entry = result.blocks.find(slice.entry);
    if (entry == result.blocks.end()) return result;
    std::vector<std::size_t> pending{entry->second};
    result.reachable[entry->second] = true;
    while (!pending.empty()) {
        const auto current = pending.back();
        pending.pop_back();
        for (const auto target : successors(slice, slice.basic_blocks[current])) {
            const auto found = result.blocks.find(target);
            if (found != result.blocks.end() && !result.reachable[found->second]) {
                result.reachable[found->second] = true;
                pending.push_back(found->second);
            }
        }
    }
    return result;
}

std::string memory_kind(std::uint32_t value) {
    if (value < 0x00400000U) return "ROM_address";
    if (value >= 0x00FF0000U && value <= 0x00FFFFFFU) return "RAM_address";
    return "other_address";
}

std::optional<std::uint32_t> immediate(const DecodedInstruction& instruction) {
    if (instruction.immediate_constants.empty()) return std::nullopt;
    return instruction.immediate_constants.front().value;
}

std::optional<std::int16_t> displacement(const DecodedInstruction& instruction) {
    if (instruction.bytes.size() < 4U) return std::nullopt;
    for (const auto& reference : instruction.unresolved_memory_references)
        if (reference.mode == 5U) return static_cast<std::int16_t>(
            (static_cast<std::uint16_t>(instruction.bytes[2]) << 8U) | instruction.bytes[3]);
    return std::nullopt;
}

void add_event(StackState& state, std::uint32_t address, std::uint32_t block,
               const char* event, const std::optional<int>& before,
               const std::optional<int>& after, const std::string& value = {}) {
    state.events.push_back({address, block, event, sp_text(before), sp_text(after), value});
}

void invalidate(StackState& state, const DecodedInstruction& instruction, std::uint32_t block,
                const std::string& event, const std::string& reason) {
    add_event(state, instruction.address, block, event.c_str(), state.sp_offset, std::nullopt, reason);
    state.sp_offset.reset();
    state.slots.clear();
    for (auto& value : state.address_registers) value.reset();
    state.blockers.push_back(reason);
}

bool is_movea_pop(const DecodedInstruction& instruction) {
    const auto opcode = instruction.opcode;
    return (opcode & 0xF1FFU) == 0x205FU && ((opcode >> 6U) & 7U) == 1U &&
        ((opcode >> 3U) & 7U) == 3U && (opcode & 7U) == 7U;
}

void push(StackState& state, const DecodedInstruction& instruction, std::uint32_t block,
          CallerStackValue value, const char* event) {
    const auto before = state.sp_offset;
    if (state.sp_offset) {
        *state.sp_offset -= 4;
        state.slots[*state.sp_offset] = std::move(value);
    } else {
        state.slots.clear();
    }
    add_event(state, instruction.address, block, event, before, state.sp_offset,
              state.sp_offset && state.slots.contains(*state.sp_offset) ? state.slots[*state.sp_offset].expression : "unknown");
}

void push_unknown(StackState& state, const DecodedInstruction& instruction, std::uint32_t block,
                  int bytes, const char* event, const std::string& value) {
    const auto before = state.sp_offset;
    if (state.sp_offset) *state.sp_offset -= bytes;
    add_event(state, instruction.address, block, event, before, state.sp_offset, value);
}

bool is_move_status_push(const DecodedInstruction& instruction) {
    const auto opcode = instruction.opcode;
    return (opcode & 0xFFC0U) == 0x40C0U && ((opcode >> 3U) & 7U) == 4U &&
        (opcode & 7U) == 7U;
}

bool is_movem_predecrement(const DecodedInstruction& instruction) {
    const auto opcode = instruction.opcode;
    return (opcode & 0xFB80U) == 0x4880U && ((opcode >> 3U) & 7U) == 4U &&
        (opcode & 7U) == 7U && instruction.bytes.size() >= 4U;
}

int movem_register_count(const DecodedInstruction& instruction) {
    const auto mask = static_cast<std::uint16_t>(instruction.bytes[2] << 8U | instruction.bytes[3]);
    int count = 0;
    for (auto value = mask; value != 0U; value = static_cast<std::uint16_t>(value >> 1U))
        count += static_cast<int>(value & 1U);
    return count;
}

void handle_instruction(StackState& state, const DecodedInstruction& instruction,
                        std::uint32_t block, const CalleeEffectReport& known_callee,
                        std::size_t& prior_calls, std::size_t& known_calls,
                        std::size_t& unknown_calls) {
    const auto opcode = instruction.opcode;
    const auto source_mode = static_cast<std::uint8_t>((opcode >> 3U) & 7U);
    const auto source_register = static_cast<std::uint8_t>(opcode & 7U);
    const auto destination_mode = static_cast<std::uint8_t>((opcode >> 6U) & 7U);
    const auto destination = static_cast<std::uint8_t>((opcode >> 9U) & 7U);
    if (instruction.flow == FlowKind::direct_call) {
        ++prior_calls;
        const bool proven = instruction.direct_target && *instruction.direct_target == known_callee.entry &&
            known_callee.stack_effect.status == "CONFIRMED";
        if (proven) {
            ++known_calls;
            add_event(state, instruction.address, block, "BSR + proven callee + RTS", state.sp_offset,
                      state.sp_offset, "return address " + hex32(instruction.address + static_cast<std::uint32_t>(instruction.bytes.size())));
        } else {
            ++unknown_calls;
            const auto target = instruction.direct_target ? hex32(*instruction.direct_target) : "UNKNOWN";
            const auto reason = "unknown direct call " + hex32(instruction.address) + " -> " + target;
            invalidate(state, instruction, block, "BSR + unknown callee", reason);
        }
        return;
    }
    if (opcode == 0x2F3CU && destination_mode == 4U && destination == 7U) {
        const auto constant = immediate(instruction);
        push(state, instruction, block,
             {constant ? hex32(*constant) : "unknown immediate", constant ? "immediate_constant" : "unknown", instruction.address},
             "MOVE.L #imm,-(A7)");
        return;
    }
    if (instruction.mnemonic == "pea") {
        if (instruction.unresolved_memory_references.empty() && !instruction.memory_references.empty()) {
            const auto& reference = instruction.memory_references.front();
            push(state, instruction, block, {hex32(reference.address), memory_kind(reference.address), instruction.address}, "PEA known address");
        } else push(state, instruction, block, {"unknown PEA value", "unknown", instruction.address}, "PEA unknown value");
        return;
    }
    if (is_move_status_push(instruction)) {
        push_unknown(state, instruction, block, 2, "MOVE.W SR,-(A7)", "unknown status word");
        return;
    }
    if (is_movem_predecrement(instruction)) {
        push_unknown(state, instruction, block, 4 * movem_register_count(instruction),
                     "MOVEM.L regs,-(A7)", "unknown register values");
        return;
    }
    if ((opcode & 0xF1F8U) == 0x2108U && destination_mode == 4U && destination == 7U && source_mode == 1U) {
        const auto source = state.address_registers[source_register];
        push(state, instruction, block, source ? *source : CallerStackValue{"unknown A" + std::to_string(source_register), "unknown", instruction.address},
             "MOVE.L An,-(A7)");
        return;
    }
    if (is_movea_pop(instruction)) {
        const auto before = state.sp_offset;
        std::optional<CallerStackValue> value;
        if (state.sp_offset) {
            const auto found = state.slots.find(*state.sp_offset);
            if (found != state.slots.end()) value = found->second;
            state.slots.erase(*state.sp_offset);
            *state.sp_offset += 4;
        } else state.slots.clear();
        state.address_registers[destination] = value;
        add_event(state, instruction.address, block, "MOVEA.L (A7)+,An", before, state.sp_offset,
                  value ? value->expression : "unknown stack value");
        return;
    }
    if ((instruction.mnemonic == "addq" || instruction.mnemonic == "subq") && destination_mode == 1U && destination == 7U) {
        const auto amount = immediate(instruction);
        if (!amount || !state.sp_offset) {
            invalidate(state, instruction, block, "stack adjust", "unknown A7 adjustment at " + hex32(instruction.address));
            return;
        }
        const auto size_code = static_cast<unsigned>((opcode >> 6U) & 3U);
        const auto delta = *amount * (size_code == 0U ? 2 : 1);
        const auto before = state.sp_offset;
        *state.sp_offset += instruction.mnemonic == "addq" ? delta : -delta;
        add_event(state, instruction.address, block, "A7 immediate adjustment", before, state.sp_offset, std::to_string(delta));
        return;
    }
    if (instruction.mnemonic == "link" || instruction.mnemonic == "unlk" ||
        ((opcode >> 12U) >= 1U && (opcode >> 12U) <= 3U && destination_mode == 1U && destination == 7U)) {
        invalidate(state, instruction, block, "stack/register transfer", "unproven A7-changing instruction at " + hex32(instruction.address));
        return;
    }
    if (instruction.mnemonic == "lea" && instruction.unresolved_memory_references.empty() && !instruction.memory_references.empty()) {
        state.address_registers[destination] = {hex32(instruction.memory_references.front().address),
                                                memory_kind(instruction.memory_references.front().address), instruction.address};
    } else if ((opcode >> 12U) >= 1U && (opcode >> 12U) <= 3U && destination_mode == 1U) {
        if (source_mode == 7U && source_register == 4U && immediate(instruction))
            state.address_registers[destination] = {hex32(*immediate(instruction)), "immediate_constant", instruction.address};
        else if (source_mode == 1U && state.address_registers[source_register])
            state.address_registers[destination] = {state.address_registers[source_register]->expression, "copied_register", instruction.address};
        else state.address_registers[destination].reset();
    } else if (instruction.mnemonic == "scc" && source_mode == 3U) {
        state.address_registers[source_register].reset();
    }
}

void collect_paths(const FlowContext& context, std::size_t block_index, std::uint32_t call_site,
                   const CalleeEffectReport& known_callee, StackState state,
                   std::set<std::size_t> active, std::vector<CallerStackPath>& paths,
                   std::size_t& prior_calls, std::size_t& known_calls, std::size_t& unknown_calls) {
    if (paths.size() >= 512U) return;
    if (!active.insert(block_index).second) return;
    const auto& block = context.slice.basic_blocks[block_index];
    state.block_starts.push_back(block.start);
    for (const auto address : block.instruction_addresses) {
        const auto* instruction = instruction_at(context.slice, address);
        if (!instruction) continue;
        if (address == call_site) {
            if (instruction->flow == FlowKind::direct_call && instruction->direct_target &&
                *instruction->direct_target == known_callee.entry && known_callee.stack_effect.status == "CONFIRMED")
                add_event(state, instruction->address, block.start, "BSR + proven callee + RTS", state.sp_offset,
                          state.sp_offset, "return address " + hex32(instruction->address + static_cast<std::uint32_t>(instruction->bytes.size())));
            else state.blockers.push_back("call-site is not a proven direct call to the audited callee at " + hex32(address));
            const auto pre_call = state.sp_offset;
            std::optional<CallerStackValue> value;
            if (pre_call) {
                const auto found = state.slots.find(*pre_call);
                if (found != state.slots.end() && found->second.kind != "unknown") value = found->second;
            }
            paths.push_back({state.block_starts, state.events, pre_call, value, "reached_call_site", state.blockers});
            return;
        }
        handle_instruction(state, *instruction, block.start, known_callee, prior_calls, known_calls, unknown_calls);
        if (instruction->flow == FlowKind::return_instruction || instruction->flow == FlowKind::unsupported) {
            state.blockers.push_back("control flow stops before call-site at " + hex32(address));
            paths.push_back({state.block_starts, state.events, std::nullopt, std::nullopt,
                             "stopped_before_call_site", state.blockers});
            return;
        }
    }
    const auto next = successors(context.slice, block);
    if (next.empty()) {
        state.blockers.push_back("no bounded successor before call-site " + hex32(call_site));
        paths.push_back({state.block_starts, state.events, std::nullopt, std::nullopt,
                         "no_bounded_successor", state.blockers});
        return;
    }
    auto ordered_next = next;
    std::stable_sort(ordered_next.begin(), ordered_next.end(), [&](const auto left, const auto right) {
        const auto left_index = context.blocks.find(left);
        const auto right_index = context.blocks.find(right);
        const auto left_distance = left_index == context.blocks.end() ? -1 : context.distance_to_call[left_index->second];
        const auto right_distance = right_index == context.blocks.end() ? -1 : context.distance_to_call[right_index->second];
        if ((left_distance >= 0) != (right_distance >= 0)) return left_distance >= 0;
        if (left_distance != right_distance) return left_distance < right_distance;
        return left < right;
    });
    for (const auto target : ordered_next) {
        const auto found = context.blocks.find(target);
        if (found != context.blocks.end() && context.reachable[found->second])
            collect_paths(context, found->second, call_site, known_callee, state, active, paths,
                          prior_calls, known_calls, unknown_calls);
    }
}

bool same_value(const CallerStackValue& left, const CallerStackValue& right) {
    return left.expression == right.expression && left.kind == right.kind;
}

void unique_events(std::vector<CallerStackEvent>& events) {
    std::sort(events.begin(), events.end(), [](const auto& left, const auto& right) {
        return std::tie(left.instruction_address, left.block_start, left.event, left.value) <
            std::tie(right.instruction_address, right.block_start, right.event, right.value);
    });
    events.erase(std::unique(events.begin(), events.end(), [](const auto& left, const auto& right) {
        return left.instruction_address == right.instruction_address && left.block_start == right.block_start &&
            left.event == right.event && left.value == right.value;
    }), events.end());
}

std::string address_class(std::uint32_t value) { return memory_kind(value); }

} // namespace

CallerStackReport analyze_caller_stack(const DecodedSlice& caller_slice,
                                       const CalleeEffectReport& known_callee) {
    CallerStackReport report{.entry = caller_slice.entry, .call_site = 0x60BCCU,
                             .callee = known_callee.entry, .window_start = caller_slice.entry,
                             .window_end = caller_slice.range_end};
    auto context = make_context(caller_slice);
    const auto call_instruction = context.instructions.find(report.call_site);
    if (call_instruction == context.instructions.end() || !context.reachable[call_instruction->second]) {
        report.blockers.push_back("call-site 0x60BCC is not reachable in bounded slice");
        report.merge_status = "NO_REACHABLE_PATH";
        report.target_results = {{0x60BFA, "unresolved_stack_value", "UNKNOWN", "unknown", "unknown", std::nullopt, "unknown", "", "call_site_unreachable"},
                                 {0x60C08, "unresolved_stack_value", "UNKNOWN", "unknown", "unknown", std::nullopt, "unknown", "", "call_site_unreachable"}};
        return report;
    }
    report.containing_blocks.push_back(caller_slice.basic_blocks[call_instruction->second].start);
    for (const auto predecessor : context.predecessors[call_instruction->second])
        if (context.reachable[predecessor]) report.predecessor_blocks.push_back(caller_slice.basic_blocks[predecessor].start);
    std::sort(report.predecessor_blocks.begin(), report.predecessor_blocks.end());
    report.predecessor_blocks.erase(std::unique(report.predecessor_blocks.begin(), report.predecessor_blocks.end()), report.predecessor_blocks.end());

    context.distance_to_call.assign(caller_slice.basic_blocks.size(), -1);
    std::vector<std::size_t> pending{call_instruction->second};
    context.distance_to_call[call_instruction->second] = 0;
    for (std::size_t index = 0; index < pending.size(); ++index) {
        const auto current = pending[index];
        for (const auto predecessor : context.predecessors[current])
            if (context.distance_to_call[predecessor] < 0) {
                context.distance_to_call[predecessor] = context.distance_to_call[current] + 1;
                pending.push_back(predecessor);
            }
    }

    std::vector<CallerStackPath> paths;
    std::size_t prior_calls = 0;
    std::size_t known_calls = 0;
    std::size_t unknown_calls = 0;
    const auto entry = context.blocks.find(caller_slice.entry);
    if (entry != context.blocks.end() && context.reachable[entry->second])
        collect_paths(context, entry->second, report.call_site, known_callee, {}, {}, paths,
                      prior_calls, known_calls, unknown_calls);
    report.cfg_paths = std::move(paths);
    report.relevant_path_count = std::count_if(report.cfg_paths.begin(), report.cfg_paths.end(), [](const auto& path) {
        return path.status == "reached_call_site";
    });
    report.prior_calls_crossed = prior_calls;
    report.known_call_effects = known_calls;
    report.unknown_call_effects = unknown_calls;
    for (const auto& path : report.cfg_paths) report.stack_events.insert(report.stack_events.end(), path.events.begin(), path.events.end());
    unique_events(report.stack_events);
    report.stack_event_count = report.stack_events.size();
    for (const auto& path : report.cfg_paths) report.blockers.insert(report.blockers.end(), path.blockers.begin(), path.blockers.end());
    std::sort(report.blockers.begin(), report.blockers.end());
    report.blockers.erase(std::unique(report.blockers.begin(), report.blockers.end()), report.blockers.end());

    std::vector<const CallerStackPath*> successful;
    for (const auto& path : report.cfg_paths)
        if (path.status == "reached_call_site") successful.push_back(&path);
    if (!successful.empty()) {
        const auto first_offset = successful.front()->pre_call_sp_offset;
        const bool same_offset = std::all_of(successful.begin(), successful.end(), [&](const auto* path) {
            return path->pre_call_sp_offset == first_offset;
        });
        report.symbolic_pre_call_sp = same_offset ? "P=" + sp_text(first_offset) : "P=PATH_DEPENDENT";
        const bool all_values = std::all_of(successful.begin(), successful.end(), [](const auto* path) {
            return path->value_at_pre_call_sp.has_value();
        });
        if (all_values && same_offset) {
            const auto& first = *successful.front()->value_at_pre_call_sp;
            const bool agrees = std::all_of(successful.begin(), successful.end(), [&](const auto* path) {
                return same_value(first, *path->value_at_pre_call_sp);
            });
            if (agrees) {
                report.value_at_pre_call_sp = first;
                report.value_kind = first.kind;
                report.merge_status = successful.size() == 1U ? "SINGLE_PATH_PROVEN" : "AGREEING_PATHS";
            } else {
                report.merge_status = "CONFLICTING_VALUES";
                report.stack_merge_conflicts = 1;
            }
        } else if (successful.size() > 1U && std::any_of(successful.begin(), successful.end(), [](const auto* path) {
                       return path->value_at_pre_call_sp.has_value();
                   })) {
            report.merge_status = "UNKNOWN_VALUE_OR_OFFSET";
        } else report.merge_status = "UNKNOWN_VALUE";
    } else report.merge_status = "NO_REACHABLE_PATH";
    if (report.value_at_pre_call_sp)
        report.provenance.push_back("memory[P] = " + report.value_at_pre_call_sp->expression +
                                    " (" + report.symbolic_pre_call_sp + ")" +
                                    " from instruction " + hex32(report.value_at_pre_call_sp->source_instruction));
    else report.provenance.push_back("memory[P] has no proven bounded value (" + report.symbolic_pre_call_sp + ")");
    report.provenance.push_back("BSR 0x60BCC pushes return address 0x60BD0 at P-4; proven callee RTS restores P");

    const auto target_result = [&](std::uint32_t address) {
        CallerStackTargetResult result{address, "unresolved_stack_value", report.symbolic_pre_call_sp,
                                       report.value_at_pre_call_sp ? report.value_at_pre_call_sp->expression : "unknown",
                                       report.value_kind, std::nullopt, "unknown", "", "unresolved_stack_value"};
        const auto* instruction = instruction_at(caller_slice, address);
        if (!report.value_at_pre_call_sp || !instruction) return result;
        const auto offset = displacement(*instruction);
        if (!offset) {
            result.status = "stack_value_proven_target_displacement_unknown";
            result.provenance = "stack value proven; target displacement not decoded";
            return result;
        }
        const auto base = std::stoul(report.value_at_pre_call_sp->expression, nullptr, 16);
        result.effective_address = static_cast<std::uint32_t>(base + *offset);
        result.address_class = address_class(*result.effective_address);
        result.status = "resolved_from_symbolic_stack_slot";
        result.provenance = "0x60BD0 MOVEA.L (A7)+,A0; A0=" + report.value_at_pre_call_sp->expression +
                            "; signed displacement=" + std::to_string(*offset);
        return result;
    };
    report.target_results = {target_result(0x60BFAU), target_result(0x60C08U)};
    report.reachable_unresolved_after = report.reachable_unresolved_before -
        std::count_if(report.target_results.begin(), report.target_results.end(), [](const auto& item) {
            return item.status == "resolved_from_symbolic_stack_slot";
        });
    report.speculative_resolutions = 0;
    return report;
}

CallerStackReport audit_caller_stack(std::span<const std::uint8_t> retail_rom) {
    const auto caller = decode_m68k_slice(retail_rom, {.entry = 0x60004U, .byte_budget = 0x1200U});
    const auto callee = audit_callee_effect(retail_rom);
    return analyze_caller_stack(caller, callee);
}

} // namespace oasis::tools
