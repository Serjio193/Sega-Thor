#include "tools/re_reachable_closure.hpp"

#include "tools/re_atlas_ranking.hpp"
#include "tools/re_reachable_stack.hpp"

#include <algorithm>
#include <array>
#include <map>
#include <queue>
#include <set>
#include <stdexcept>
#include <tuple>

namespace oasis::tools {
namespace {

using Register = std::uint8_t;

struct Transfer {
    enum class Kind { none, set, copy, add, sub, pop_stack, unsupported } kind{Kind::none};
    Register destination{};
    Register source{};
    std::uint32_t value{};
    std::string operation;
};

struct Path {
    std::optional<std::uint32_t> value;
    std::vector<ClosureDefinition> definitions;
    std::string stop;
};

struct Context {
    const DecodedSlice& slice;
    std::map<std::uint32_t, std::size_t> block_by_address;
    std::vector<std::vector<std::size_t>> predecessors;
    std::vector<bool> reachable;
};

std::uint16_t read16(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    return static_cast<std::uint16_t>((bytes.at(offset) << 8U) | bytes.at(offset + 1U));
}

std::uint32_t read32(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    return (static_cast<std::uint32_t>(read16(bytes, offset)) << 16U) | read16(bytes, offset + 2U);
}

std::int32_t signed_value(std::uint32_t value, unsigned width) {
    return width == 2U ? static_cast<std::int16_t>(value) : static_cast<std::int32_t>(value);
}

const DecodedInstruction* instruction_at(const DecodedSlice& slice, std::uint32_t address) {
    const auto found = std::find_if(slice.instructions.begin(), slice.instructions.end(),
                                    [=](const auto& item) { return item.address == address; });
    return found == slice.instructions.end() ? nullptr : &*found;
}

std::optional<std::uint32_t> block_start(const Context& context, std::uint32_t address) {
    const auto found = context.block_by_address.find(address);
    if (found == context.block_by_address.end()) return std::nullopt;
    return context.slice.basic_blocks[found->second].start;
}

bool has_block(const DecodedSlice& slice, std::uint32_t start) {
    return std::any_of(slice.basic_blocks.begin(), slice.basic_blocks.end(),
                       [=](const auto& block) { return block.start == start; });
}

std::vector<std::uint32_t> successors(const DecodedSlice& slice, const BasicBlock& block) {
    if (block.instruction_addresses.empty()) return {};
    const auto* last = instruction_at(slice, block.instruction_addresses.back());
    if (!last) return {};
    std::vector<std::uint32_t> result;
    for (const auto& edge : slice.control_flow)
        if (edge.source == last->address && has_block(slice, edge.target)) result.push_back(edge.target);
    const bool fallthrough = last->flow == FlowKind::direct_call ||
        (last->flow == FlowKind::direct_branch && last->mnemonic != "bra") || last->flow == FlowKind::none;
    if (fallthrough && has_block(slice, block.end)) result.push_back(block.end);
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

Context make_context(const DecodedSlice& slice) {
    Context context{slice, {}, std::vector<std::vector<std::size_t>>(slice.basic_blocks.size()),
                    std::vector<bool>(slice.basic_blocks.size(), false)};
    for (std::size_t i = 0; i < slice.basic_blocks.size(); ++i)
        for (const auto address : slice.basic_blocks[i].instruction_addresses) context.block_by_address[address] = i;
    for (std::size_t i = 0; i < slice.basic_blocks.size(); ++i)
        for (const auto target : successors(slice, slice.basic_blocks[i])) {
            const auto found = std::find_if(slice.basic_blocks.begin(), slice.basic_blocks.end(),
                                            [=](const auto& block) { return block.start == target; });
            if (found != slice.basic_blocks.end())
                context.predecessors[static_cast<std::size_t>(std::distance(slice.basic_blocks.begin(), found))].push_back(i);
        }
    const auto entry = context.block_by_address.find(slice.entry);
    if (entry == context.block_by_address.end()) return context;
    std::queue<std::size_t> pending;
    context.reachable[entry->second] = true;
    pending.push(entry->second);
    while (!pending.empty()) {
        const auto current = pending.front();
        pending.pop();
        for (const auto target : successors(slice, slice.basic_blocks[current])) {
            const auto found = context.block_by_address.find(target);
            if (found != context.block_by_address.end() && !context.reachable[found->second]) {
                context.reachable[found->second] = true;
                pending.push(found->second);
            }
        }
    }
    return context;
}

Transfer recognize_transfer(const DecodedInstruction& instruction) {
    const auto opcode = instruction.opcode;
    const auto destination = static_cast<Register>((opcode >> 9U) & 7U);
    const auto source_mode = static_cast<Register>((opcode >> 3U) & 7U);
    const auto source_reg = static_cast<Register>(opcode & 7U);
    const auto destination_mode = static_cast<Register>((opcode >> 6U) & 7U);
    if ((opcode & 0xF1FFU) == 0x207CU || (opcode & 0xF1FFU) == 0x307CU) {
        const auto width = (opcode & 0x3000U) == 0x2000U ? 4U : 2U;
        return {Transfer::Kind::set, destination, 0, static_cast<std::uint32_t>(signed_value(width == 4U ? read32(instruction.bytes, 2U) : read16(instruction.bytes, 2U), width)),
                "MOVEA immediate -> A" + std::to_string(destination)};
    }
    if ((opcode & 0xF1C0U) == 0x41C0U && source_mode == 7U && source_reg <= 2U) {
        std::uint32_t value = 0;
        std::string operation;
        if (source_reg == 0U) {
            value = static_cast<std::uint32_t>(static_cast<std::int16_t>(read16(instruction.bytes, 2U)));
            operation = "LEA absolute-word -> A" + std::to_string(destination);
        } else if (source_reg == 1U) {
            value = read32(instruction.bytes, 2U);
            operation = "LEA absolute-long -> A" + std::to_string(destination);
        } else {
            value = static_cast<std::uint32_t>(instruction.address + 2U +
                                               static_cast<std::int16_t>(read16(instruction.bytes, 2U)));
            operation = "LEA PC-displacement -> A" + std::to_string(destination);
        }
        return {Transfer::Kind::set, destination, 0, value, std::move(operation)};
    }
    if ((opcode & 0xF1FFU) == 0xD0FCU || (opcode & 0xF1FFU) == 0xD1FCU ||
        (opcode & 0xF1FFU) == 0x90FCU || (opcode & 0xF1FFU) == 0x91FCU) {
        const bool subtract = (opcode & 0xF000U) == 0x9000U;
        const unsigned width = (opcode & 0x0100U) != 0U ? 4U : 2U;
        const auto raw = width == 4U ? read32(instruction.bytes, 2U) : read16(instruction.bytes, 2U);
        return {subtract ? Transfer::Kind::sub : Transfer::Kind::add, destination, 0,
                static_cast<std::uint32_t>(signed_value(raw, width)),
                std::string(subtract ? "SUBA immediate -> A" : "ADDA immediate -> A") + std::to_string(destination)};
    }
    if ((opcode & 0xF000U) == 0x2000U && destination_mode == 1U && source_mode == 1U)
        return {Transfer::Kind::copy, destination, source_reg, 0,
                "MOVEA A" + std::to_string(source_reg) + " -> A" + std::to_string(destination)};
    if ((opcode & 0xF1FFU) == 0x205FU && destination_mode == 1U && source_mode == 3U && source_reg == 7U)
        return {Transfer::Kind::pop_stack, destination, 0, 0,
                "MOVEA.L (A7)+ -> A" + std::to_string(destination) + "; A7 += 4"};
    if ((opcode & 0xF000U) >= 0x2000U && (opcode & 0xF000U) <= 0x3000U && destination_mode == 1U)
        return {Transfer::Kind::unsupported, destination, 0, 0, "unsupported MOVEA transfer"};
    if ((opcode & 0xF100U) == 0x5000U && source_mode == 1U) {
        const auto immediate = static_cast<std::uint32_t>(((opcode >> 9U) & 7U) == 0U ? 8U : (opcode >> 9U) & 7U);
        const bool subtract = (opcode & 0x0100U) != 0U;
        return {subtract ? Transfer::Kind::sub : Transfer::Kind::add, source_reg, 0, immediate,
                std::string(subtract ? "SUBQ -> A" : "ADDQ -> A") + std::to_string(source_reg)};
    }
    return {};
}

bool is_call(const DecodedInstruction& instruction) {
    return instruction.flow == FlowKind::direct_call || instruction.flow == FlowKind::indirect_call;
}

std::optional<Register> writes_address_register(const DecodedInstruction& instruction) {
    const auto opcode = instruction.opcode;
    if ((opcode & 0xF000U) >= 0x2000U && (opcode & 0xF000U) <= 0x3000U &&
        ((opcode >> 6U) & 7U) == 1U) return static_cast<Register>((opcode >> 9U) & 7U);
    if ((opcode & 0xF1C0U) == 0x41C0U) return static_cast<Register>((opcode >> 9U) & 7U);
    if (instruction.mnemonic == "link") return 7U;
    if (instruction.mnemonic == "unlk") return static_cast<Register>(opcode & 7U);
    return std::nullopt;
}

ClosureDefinition definition_for(const DecodedInstruction& instruction, std::uint32_t block_start,
                                 const Transfer& transfer, std::optional<std::uint32_t> value) {
    return {instruction.address, block_start, transfer.operation, value, transfer.kind != Transfer::Kind::unsupported};
}

void append_definition(Path& path, const ClosureDefinition& definition) {
    path.definitions.push_back(definition);
    if (definition.value) path.value = definition.value;
}

std::vector<Path> resolve_before(const Context& context, std::size_t block_index, std::size_t position,
                                Register register_index, std::set<std::tuple<std::size_t, std::size_t, Register>>& seen);

std::vector<Path> resolve_transfer(const Context& context, std::size_t block_index, std::size_t position,
                                  Register register_index, const DecodedInstruction& instruction,
                                  const Transfer& transfer, std::set<std::tuple<std::size_t, std::size_t, Register>>& seen) {
    if (transfer.kind == Transfer::Kind::set && transfer.destination == register_index)
        return {{transfer.value, {definition_for(instruction, context.slice.basic_blocks[block_index].start, transfer, transfer.value)}, {}}};
    if (transfer.destination != register_index) return {};
    if (transfer.kind == Transfer::Kind::unsupported)
        return {{std::nullopt, {definition_for(instruction, context.slice.basic_blocks[block_index].start, transfer, std::nullopt)}, "unsupported_transfer"}};
    if (transfer.kind == Transfer::Kind::pop_stack) {
        const auto stack = analyze_bounded_movea_postincrement(context.slice, instruction.address, transfer.destination);
        Path path{stack.value, stack.provenance, stack.value ? std::string{} : stack.status};
        path.definitions.push_back(definition_for(instruction, context.slice.basic_blocks[block_index].start, transfer, stack.value));
        return {path};
    }
    if (transfer.kind != Transfer::Kind::copy && transfer.kind != Transfer::Kind::add && transfer.kind != Transfer::Kind::sub)
        return {};
    const auto source = transfer.kind == Transfer::Kind::copy ? transfer.source : transfer.destination;
    auto paths = resolve_before(context, block_index, position, source, seen);
    for (auto& path : paths) {
        if (path.value && transfer.kind == Transfer::Kind::copy) {
            append_definition(path, definition_for(instruction, context.slice.basic_blocks[block_index].start, transfer, path.value));
        } else if (path.value) {
            const auto value = transfer.kind == Transfer::Kind::add ? *path.value + transfer.value : *path.value - transfer.value;
            append_definition(path, definition_for(instruction, context.slice.basic_blocks[block_index].start, transfer, value));
        } else {
            path.definitions.push_back(definition_for(instruction, context.slice.basic_blocks[block_index].start, transfer, std::nullopt));
        }
    }
    return paths;
}

std::vector<Path> resolve_before(const Context& context, std::size_t block_index, std::size_t position,
                                Register register_index, std::set<std::tuple<std::size_t, std::size_t, Register>>& seen) {
    if (!seen.insert({block_index, position, register_index}).second) return {{std::nullopt, {}, "other"}};
    const auto& block = context.slice.basic_blocks[block_index];
    for (std::size_t i = position; i > 0U; --i) {
        const auto address = block.instruction_addresses[i - 1U];
        const auto* instruction = instruction_at(context.slice, address);
        if (!instruction) continue;
        if (is_call(*instruction)) {
            auto prior = resolve_before(context, block_index, i - 1U, register_index, seen);
            if (prior.empty()) prior.push_back({});
            for (auto& path : prior) {
                path.value = std::nullopt;
                path.stop = "call_clobber";
            }
            return prior;
        }
        const auto transfer = recognize_transfer(*instruction);
        if (transfer.kind != Transfer::Kind::none && transfer.destination == register_index)
            return resolve_transfer(context, block_index, i - 1U, register_index, *instruction, transfer, seen);
        const auto written = writes_address_register(*instruction);
        if (written && *written == register_index)
            return {{std::nullopt, {definition_for(*instruction, block.start, transfer, std::nullopt)}, "unsupported_transfer"}};
    }
    if (context.predecessors[block_index].empty())
        return {{std::nullopt, {}, block.start == context.slice.entry ? "entry_state_unknown" : "other"}};
    std::vector<Path> result;
    for (const auto predecessor : context.predecessors[block_index]) {
        if (!context.reachable[predecessor]) continue;
        auto branch_seen = seen;
        auto paths = resolve_before(context, predecessor, context.slice.basic_blocks[predecessor].instruction_addresses.size(), register_index, branch_seen);
        result.insert(result.end(), paths.begin(), paths.end());
    }
    return result.empty() ? std::vector<Path>{{std::nullopt, {}, "other"}} : result;
}

bool all_concrete_same(const std::vector<Path>& paths, std::uint32_t& value) {
    if (paths.empty() || std::any_of(paths.begin(), paths.end(), [](const auto& path) { return !path.value; })) return false;
    value = *paths.front().value;
    return std::all_of(paths.begin(), paths.end(), [&](const auto& path) { return *path.value == value; });
}

ClosureReason reason_for(ResolutionStatus initial_status, const std::vector<Path>& paths) {
    std::uint32_t ignored = 0;
    if (paths.size() > 1U && std::any_of(paths.begin(), paths.end(), [&](const auto& path) {
        return path.value && !all_concrete_same(paths, ignored);
    })) return ClosureReason::conflicting_cfg_merge;
    if (std::any_of(paths.begin(), paths.end(), [](const auto& path) { return path.stop == "call_clobber"; })) return ClosureReason::call_clobber;
    if (std::any_of(paths.begin(), paths.end(), [](const auto& path) { return path.stop == "unsupported_transfer"; })) return ClosureReason::unsupported_transfer;
    if (std::any_of(paths.begin(), paths.end(), [](const auto& path) { return path.stop == "entry_state_unknown"; })) return ClosureReason::entry_state_unknown;
    if (std::any_of(paths.begin(), paths.end(), [](const auto& path) { return path.stop == "conflicting_stack_merge"; }))
        return ClosureReason::conflicting_cfg_merge;
    if (std::any_of(paths.begin(), paths.end(), [](const auto& path) {
        return path.stop.rfind("stack_value_unknown", 0U) == 0U;
    })) return ClosureReason::other;
    if (initial_status == ResolutionStatus::unresolved_cfg_merge) return ClosureReason::conflicting_cfg_merge;
    if (initial_status == ResolutionStatus::unresolved_unsupported_transfer) return ClosureReason::unsupported_transfer;
    return initial_status == ResolutionStatus::unresolved_unknown_base ? ClosureReason::unknown_base : ClosureReason::other;
}

void add_count(std::vector<ClosureReasonCount>& counts, ClosureReason reason) {
    const auto key = closure_reason_name(reason);
    const auto found = std::find_if(counts.begin(), counts.end(), [&](const auto& item) { return item.key == key; });
    if (found == counts.end()) counts.push_back({key, 1}); else ++found->count;
}

std::optional<std::int16_t> displacement(const DecodedInstruction& instruction) {
    for (const auto& item : instruction.unresolved_memory_references)
        if (item.mode == 5U) {
            const auto opcode = instruction.opcode;
            const auto source_mode = static_cast<std::uint8_t>((opcode >> 3U) & 7U);
            if (source_mode == 5U && (opcode >> 12U) <= 3U) return static_cast<std::int16_t>(read16(instruction.bytes, 2U));
            if (instruction.bytes.size() >= 4U) return static_cast<std::int16_t>(read16(instruction.bytes, 2U));
        }
    return std::nullopt;
}

std::string operand(const ReachableClosureItem& item) {
    return "A" + std::to_string(item.base_register) + " + signed displacement";
}

ReachableClosureReport analyze_entry(const AtlasEntry& atlas_entry, const DecodedSlice& slice) {
    const auto initial = resolve_decoded_displacements(atlas_entry, slice);
    const auto context = make_context(slice);
    ReachableClosureReport report{.target_entry = slice.entry, .window_start = slice.entry, .window_end = slice.range_end,
                                  .exact_reachable_unresolved_count = 0, .reachable_unresolved_before = 0,
                                  .transfer_rule = "MOVEA.L (A7)+,An: source mode 3/A7, destination mode 1/An, longword, value=memory[old A7], A7 += 4",
                                  .dynamic_scenario = "not attempted: static closure has more than a few refs"};
    for (const auto& item : initial.items) {
        if (item.status == ResolutionStatus::resolved) continue;
        const auto found = context.block_by_address.find(item.instruction_address);
        if (found == context.block_by_address.end() || !context.reachable[found->second]) continue;
        const auto* instruction = instruction_at(slice, item.instruction_address);
        if (!instruction) continue;
        ReachableClosureItem result{.instruction_address = item.instruction_address, .block_start = item.block_start,
                                    .opcode = instruction->opcode, .bytes = instruction->bytes,
                                    .mnemonic = instruction->mnemonic, .addressing_modes = instruction->addressing_modes,
                                    .operand = {}, .base_register = item.base_register, .displacement = item.displacement,
                                    .initial_status = item.status, .current_unresolved_reason = item.reason,
                                    .prior_closure_reason = (item.instruction_address == 0x60BFAU || item.instruction_address == 0x60C08U) ?
                                        "unsupported_transfer" : ""};
        for (const auto predecessor : context.predecessors[found->second])
            if (context.reachable[predecessor]) result.cfg_predecessors.push_back(slice.basic_blocks[predecessor].start);
        std::sort(result.cfg_predecessors.begin(), result.cfg_predecessors.end());
        result.cfg_predecessors.erase(std::unique(result.cfg_predecessors.begin(), result.cfg_predecessors.end()), result.cfg_predecessors.end());
        result.operand = operand(result);
        const auto backward = analyze_bounded_backward_register(slice, item.instruction_address, item.base_register);
        result.reason = backward.reason;
        result.evidence = "static_bounded_backward_slice";
        result.confidence = result.reason == ClosureReason::conflicting_cfg_merge ? "HIGH" : "MEDIUM";
        result.last_known_definitions = backward.definitions;
        result.provenance = backward.provenance;
        result.stack_status = backward.stack_status;
        result.a7_before = backward.a7_before;
        result.a7_increment_bytes = backward.a7_increment_bytes;
        result.stack_provenance = backward.stack_provenance;
        std::sort(result.last_known_definitions.begin(), result.last_known_definitions.end(), [](const auto& left, const auto& right) {
            return std::tie(left.instruction_address, left.block_start, left.operation) < std::tie(right.instruction_address, right.block_start, right.operation);
        });
        result.last_known_definitions.erase(std::unique(result.last_known_definitions.begin(), result.last_known_definitions.end(),
            [](const auto& left, const auto& right) { return left.instruction_address == right.instruction_address && left.block_start == right.block_start && left.operation == right.operation; }), result.last_known_definitions.end());
        std::sort(result.provenance.begin(), result.provenance.end(), [](const auto& left, const auto& right) {
            return std::tie(left.instruction_address, left.block_start, left.operation, left.value) < std::tie(right.instruction_address, right.block_start, right.operation, right.value);
        });
        result.provenance.erase(std::unique(result.provenance.begin(), result.provenance.end(), [](const auto& left, const auto& right) {
            return left.instruction_address == right.instruction_address && left.block_start == right.block_start && left.operation == right.operation && left.value == right.value;
        }), result.provenance.end());
        std::uint32_t value = 0;
        if (backward.value) {
            value = *backward.value;
            result.effective_address = value + item.displacement;
            result.address_class = result.effective_address.value() < 0x00400000U ? EffectiveAddressClass::rom :
                result.effective_address.value() >= 0x00FF0000U && result.effective_address.value() <= 0x00FFFFFFU ? EffectiveAddressClass::ram : EffectiveAddressClass::outside_known_address_space;
            result.nearest_proven_register_state = result.provenance.empty() ? std::nullopt : std::optional(result.provenance.back());
            result.reason = ClosureReason::other;
            result.confidence = "HIGH";
            ++report.newly_resolved;
            if (result.address_class == EffectiveAddressClass::ram) ++report.ram_effective_address_count;
            if (result.address_class == EffectiveAddressClass::rom) ++report.rom_effective_address_count;
        } else if (!result.provenance.empty()) {
            result.nearest_proven_register_state = result.provenance.back();
        }
        report.items.push_back(std::move(result));
    }
    std::sort(report.items.begin(), report.items.end(), [](const auto& left, const auto& right) { return left.instruction_address < right.instruction_address; });
    report.exact_reachable_unresolved_count = report.items.size();
    report.reachable_unresolved_before = report.items.size();
    report.reachable_unresolved_after = report.reachable_unresolved_before - report.newly_resolved;
    report.provenance_failures = std::count_if(report.items.begin(), report.items.end(), [](const auto& item) {
        return item.effective_address && item.provenance.empty();
    });
    for (const auto& item : report.items) add_count(report.reason_counts, item.reason);
    std::sort(report.reason_counts.begin(), report.reason_counts.end(), [](const auto& left, const auto& right) { return left.key < right.key; });
    return report;
}

std::size_t ranking_value(const AtlasRankingReport& report, const std::string& dimension, const std::string& key) {
    const auto found = std::find_if(report.groups.begin(), report.groups.end(), [&](const auto& item) { return item.dimension == dimension && item.key == key; });
    return found == report.groups.end() ? 0U : found->frequency;
}

} // namespace

BackwardAnalysis analyze_bounded_backward_register(const DecodedSlice& slice,
                                                   std::uint32_t instruction_address,
                                                   std::uint8_t base_register) {
    const auto context = make_context(slice);
    const auto found = context.block_by_address.find(instruction_address);
    if (found == context.block_by_address.end() || !context.reachable[found->second]) {
        return {.reason = ClosureReason::other};
    }
    const auto& addresses = slice.basic_blocks[found->second].instruction_addresses;
    const auto position = std::find(addresses.begin(), addresses.end(), instruction_address);
    if (position == addresses.end()) return {.reason = ClosureReason::other};
    std::set<std::tuple<std::size_t, std::size_t, Register>> seen;
    const auto paths = resolve_before(context, found->second,
                                      static_cast<std::size_t>(std::distance(addresses.begin(), position)),
                                      base_register, seen);
    BackwardAnalysis result;
    result.reason = reason_for(ResolutionStatus::unresolved_unknown_base, paths);
    std::uint32_t value = 0;
    if (all_concrete_same(paths, value)) {
        result.reason = ClosureReason::other;
        result.value = value;
    }
    for (const auto& path : paths) {
        result.definitions.insert(result.definitions.end(), path.definitions.begin(), path.definitions.end());
        for (const auto& definition : path.definitions)
            if (definition.value) result.provenance.push_back({definition.instruction_address, definition.block_start,
                                                                  definition.operation, *definition.value});
    }
    const auto stack_definition = std::find_if(result.definitions.begin(), result.definitions.end(),
                                               [](const auto& definition) {
                                                   return definition.operation.rfind("MOVEA.L (A7)+ -> A", 0U) == 0U;
                                               });
    if (stack_definition != result.definitions.end()) {
        const auto stack = analyze_bounded_movea_postincrement(slice, stack_definition->instruction_address,
                                                               static_cast<std::uint8_t>((instruction_at(slice, stack_definition->instruction_address)->opcode >> 9U) & 7U));
        result.stack_status = stack.status;
        result.a7_before = stack.a7_before;
        result.a7_increment_bytes = stack.a7_increment_bytes;
        result.stack_provenance = stack.provenance;
    }
    std::sort(result.definitions.begin(), result.definitions.end(), [](const auto& left, const auto& right) {
        return std::tie(left.instruction_address, left.block_start, left.operation) <
            std::tie(right.instruction_address, right.block_start, right.operation);
    });
    result.definitions.erase(std::unique(result.definitions.begin(), result.definitions.end(), [](const auto& left, const auto& right) {
        return left.instruction_address == right.instruction_address && left.block_start == right.block_start && left.operation == right.operation;
    }), result.definitions.end());
    std::sort(result.provenance.begin(), result.provenance.end(), [](const auto& left, const auto& right) {
        return std::tie(left.instruction_address, left.block_start, left.operation, left.value) <
            std::tie(right.instruction_address, right.block_start, right.operation, right.value);
    });
    result.provenance.erase(std::unique(result.provenance.begin(), result.provenance.end(), [](const auto& left, const auto& right) {
        return left.instruction_address == right.instruction_address && left.block_start == right.block_start && left.operation == right.operation && left.value == right.value;
    }), result.provenance.end());
    return result;
}

ReachableClosureReport audit_reachable_unresolved(const AtlasEntry& atlas_entry, const DecodedSlice& slice) {
    return analyze_entry(atlas_entry, slice);
}

ReachableClosureReport audit_reachable_unresolved(const AtlasReport& atlas, std::span<const std::uint8_t> retail_rom) {
    const auto found = std::find_if(atlas.entries.begin(), atlas.entries.end(), [](const auto& entry) { return entry.start == 0x60004U; });
    if (found == atlas.entries.end()) throw std::invalid_argument("Atlas does not contain bounded 0x60004 entry");
    const auto slice = decode_m68k_slice(retail_rom, {.entry = 0x60004U, .byte_budget = 0x1200U});
    auto report = analyze_entry(*found, slice);
    const auto ranking = rank_atlas_unresolved(atlas);
    report.raw_static_unresolved = ranking.atlas_unresolved_reference_count;
    report.raw_displacement_backlog = ranking_value(ranking, "addressing_mode", "address_displacement");
    report.nonreachable_unresolved = resolve_bounded_displacements(atlas, retail_rom).still_unresolved - report.exact_reachable_unresolved_count;
    report.speculative_resolutions = 0;
    report.atlas_unresolved_before = ranking.atlas_unresolved_reference_count;
    report.atlas_unresolved_after = report.atlas_unresolved_before - report.newly_resolved;
    report.ranking_displacement_before = report.raw_displacement_backlog;
    report.ranking_displacement_after = report.raw_displacement_backlog - report.newly_resolved;
    return report;
}

std::string closure_reason_name(ClosureReason reason) {
    switch (reason) {
    case ClosureReason::unknown_base: return "unknown_base";
    case ClosureReason::conflicting_cfg_merge: return "conflicting_cfg_merge";
    case ClosureReason::unsupported_transfer: return "unsupported_transfer";
    case ClosureReason::call_clobber: return "call_clobber";
    case ClosureReason::entry_state_unknown: return "entry_state_unknown";
    case ClosureReason::other: return "other";
    }
    return "other";
}

} // namespace oasis::tools
