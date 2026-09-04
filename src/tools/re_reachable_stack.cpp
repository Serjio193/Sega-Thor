#include "tools/re_reachable_stack.hpp"

#include <algorithm>
#include <map>
#include <queue>
#include <set>

namespace oasis::tools {
namespace {

struct StackPath {
    std::optional<std::uint32_t> value;
    std::vector<ClosureDefinition> definitions;
    int depth{};
    std::string stop;
    std::optional<std::uint32_t> a7_before;
};

struct StackContext {
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

const DecodedInstruction* instruction_at(const DecodedSlice& slice, std::uint32_t address) {
    const auto found = std::find_if(slice.instructions.begin(), slice.instructions.end(),
                                    [=](const auto& item) { return item.address == address; });
    return found == slice.instructions.end() ? nullptr : &*found;
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

StackContext make_context(const DecodedSlice& slice) {
    StackContext context{slice, {}, std::vector<std::vector<std::size_t>>(slice.basic_blocks.size()),
                         std::vector<bool>(slice.basic_blocks.size(), false)};
    for (std::size_t i = 0; i < slice.basic_blocks.size(); ++i)
        for (const auto address : slice.basic_blocks[i].instruction_addresses) context.block_by_address[address] = i;
    for (std::size_t i = 0; i < slice.basic_blocks.size(); ++i)
        for (const auto target : successors(slice, slice.basic_blocks[i])) {
            const auto found = context.block_by_address.find(target);
            if (found != context.block_by_address.end()) context.predecessors[found->second].push_back(i);
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

bool is_movea_pop(const DecodedInstruction& instruction, std::uint8_t destination) {
    const auto opcode = instruction.opcode;
    return (opcode & 0xF1FFU) == 0x205FU &&
        ((opcode >> 9U) & 7U) == destination && ((opcode >> 6U) & 7U) == 1U &&
        ((opcode >> 3U) & 7U) == 3U && (opcode & 7U) == 7U;
}

std::optional<std::uint32_t> push_value(const DecodedInstruction& instruction, std::uint8_t& source_register,
                                        std::string& operation) {
    const auto opcode = instruction.opcode;
    const auto source_mode = static_cast<std::uint8_t>((opcode >> 3U) & 7U);
    const auto source_reg = static_cast<std::uint8_t>(opcode & 7U);
    const auto destination_mode = static_cast<std::uint8_t>((opcode >> 6U) & 7U);
    const auto destination_reg = static_cast<std::uint8_t>((opcode >> 9U) & 7U);
    if (opcode == 0x2F3CU && instruction.bytes.size() >= 6U) {
        operation = "MOVE.L #imm,-(A7)";
        return read32(instruction.bytes, 2U);
    }
    if (opcode == 0x4878U && instruction.bytes.size() >= 4U) {
        operation = "PEA absolute-word";
        return static_cast<std::uint32_t>(static_cast<std::int16_t>(read16(instruction.bytes, 2U)));
    }
    if (opcode == 0x4879U && instruction.bytes.size() >= 6U) {
        operation = "PEA absolute-long";
        return read32(instruction.bytes, 2U);
    }
    if (opcode == 0x486AU && instruction.bytes.size() >= 4U) {
        operation = "PEA PC-displacement";
        return static_cast<std::uint32_t>(instruction.address + 2U + static_cast<std::int16_t>(read16(instruction.bytes, 2U)));
    }
    if ((opcode & 0xF1F8U) == 0x2108U && destination_mode == 4U && destination_reg == 7U &&
        source_mode == 1U) {
        source_register = source_reg;
        operation = "MOVE.L A" + std::to_string(source_reg) + ",-(A7)";
        return std::nullopt;
    }
    return std::nullopt;
}

bool is_stack_write(const DecodedInstruction& instruction) {
    const auto opcode = instruction.opcode;
    return (opcode & 0xF000U) == 0x2000U && ((opcode >> 6U) & 7U) == 4U &&
        ((opcode >> 9U) & 7U) == 7U;
}

std::vector<StackPath> resolve_stack_before(const StackContext& context, std::size_t block_index,
                                             std::size_t position,
                                             std::set<std::pair<std::size_t, std::size_t>>& seen) {
    if (!seen.insert({block_index, position}).second)
        return {{std::nullopt, {}, 0, "other", std::nullopt}};
    const auto& block = context.slice.basic_blocks[block_index];
    const auto path_for_push = [&](const ClosureDefinition& definition, std::uint32_t value) {
        const auto a7_state = analyze_bounded_backward_register(context.slice, definition.instruction_address, 7U);
        if (!a7_state.value || *a7_state.value < 4U)
            return StackPath{std::nullopt, {definition}, 0, "stack_a7_unknown", std::nullopt};
        return StackPath{value, {definition}, 1, {}, *a7_state.value - 4U};
    };
    for (std::size_t i = position; i > 0U; --i) {
        const auto address = block.instruction_addresses[i - 1U];
        const auto* instruction = instruction_at(context.slice, address);
        if (!instruction) continue;
        if (instruction->flow == FlowKind::direct_call || instruction->flow == FlowKind::indirect_call)
            return {{std::nullopt, {}, 0, "stack_value_unknown_call_boundary"}};
        const bool pea = instruction->opcode == 0x486AU || instruction->opcode == 0x4878U || instruction->opcode == 0x4879U;
        if (is_stack_write(*instruction) || pea) {
            std::uint8_t source_register = 0;
            std::string operation;
            const auto value = push_value(*instruction, source_register, operation);
            ClosureDefinition definition{instruction->address, block.start, operation, value, true};
            if (value) return {path_for_push(definition, *value)};
            if (operation.rfind("MOVE.L A", 0U) == 0U) {
                const auto register_state = analyze_bounded_backward_register(context.slice, instruction->address,
                                                                                source_register);
                definition.value = register_state.value;
                if (definition.value) return {path_for_push(definition, *definition.value)};
            }
            return {{std::nullopt, {definition}, 0, "stack_value_unknown"}};
        }
        if (is_movea_pop(*instruction, static_cast<std::uint8_t>((instruction->opcode >> 9U) & 7U)))
            return {{std::nullopt, {}, 0, "stack_value_unknown_prior_pop"}};
        if (instruction->mnemonic == "link" || instruction->mnemonic == "unlk")
            return {{std::nullopt, {}, 0, "stack_value_unknown_frame_change"}};
    }
    if (context.predecessors[block_index].empty())
        return {{std::nullopt, {}, 0, block.start == context.slice.entry ? "entry_state_unknown" : "other", std::nullopt}};
    std::vector<StackPath> result;
    for (const auto predecessor : context.predecessors[block_index]) {
        if (!context.reachable[predecessor]) continue;
        auto branch_seen = seen;
        auto paths = resolve_stack_before(context, predecessor,
                                          context.slice.basic_blocks[predecessor].instruction_addresses.size(), branch_seen);
        result.insert(result.end(), paths.begin(), paths.end());
    }
    if (result.empty()) return {{std::nullopt, {}, 0, "other", std::nullopt}};
    return result;
}

void sort_definitions(std::vector<ClosureDefinition>& definitions) {
    std::sort(definitions.begin(), definitions.end(), [](const auto& left, const auto& right) {
        return std::tie(left.instruction_address, left.block_start, left.operation) <
            std::tie(right.instruction_address, right.block_start, right.operation);
    });
    definitions.erase(std::unique(definitions.begin(), definitions.end(), [](const auto& left, const auto& right) {
        return left.instruction_address == right.instruction_address && left.block_start == right.block_start &&
            left.operation == right.operation;
    }), definitions.end());
}

} // namespace

BoundedStackAnalysis analyze_bounded_movea_postincrement(const DecodedSlice& slice,
                                                         std::uint32_t instruction_address,
                                                         std::uint8_t destination_register) {
    BoundedStackAnalysis result;
    result.a7_increment_bytes = 4U;
    const auto context = make_context(slice);
    const auto found = context.block_by_address.find(instruction_address);
    if (found == context.block_by_address.end() || !context.reachable[found->second]) {
        result.status = "target_not_reachable";
        return result;
    }
    const auto& addresses = slice.basic_blocks[found->second].instruction_addresses;
    const auto position = std::find(addresses.begin(), addresses.end(), instruction_address);
    const auto* pop = instruction_at(slice, instruction_address);
    if (position == addresses.end() || !pop || !is_movea_pop(*pop, destination_register)) {
        result.status = "not_exact_movea_l_postincrement_a7";
        result.a7_increment_bytes = 0U;
        return result;
    }
    ClosureDefinition pop_definition{instruction_address, slice.basic_blocks[found->second].start,
                                    "MOVEA.L (A7)+ -> A" + std::to_string(destination_register) + "; A7 += 4", std::nullopt, true};
    result.definitions.push_back(pop_definition);
    result.provenance.push_back(pop_definition);
    std::set<std::pair<std::size_t, std::size_t>> seen;
    const auto paths = resolve_stack_before(context, found->second,
                                            static_cast<std::size_t>(std::distance(addresses.begin(), position)), seen);
    bool all_concrete = !paths.empty() && std::all_of(paths.begin(), paths.end(), [](const auto& path) { return path.value.has_value(); });
    if (all_concrete) {
        const auto value = *paths.front().value;
        all_concrete = std::all_of(paths.begin(), paths.end(), [&](const auto& path) {
            return *path.value == value && path.depth == paths.front().depth && path.a7_before == paths.front().a7_before;
        });
        if (all_concrete) {
            result.value = value;
            result.a7_before = paths.front().a7_before;
            result.reason = ClosureReason::other;
            result.status = "proven_concrete_stack_value";
        }
    }
    if (!result.value) {
        const bool conflict = paths.size() > 1U && std::any_of(paths.begin(), paths.end(), [&](const auto& path) {
            return path.value && (*path.value != (paths.front().value.value_or(0U)) ||
                                  path.a7_before != paths.front().a7_before);
        });
        result.reason = conflict ? ClosureReason::conflicting_cfg_merge : ClosureReason::other;
        result.status = conflict ? "conflicting_stack_merge" : (paths.empty() ? "stack_value_unknown" : paths.front().stop);
    }
    for (const auto& path : paths) result.provenance.insert(result.provenance.end(), path.definitions.begin(), path.definitions.end());
    result.definitions.insert(result.definitions.end(), result.provenance.begin(), result.provenance.end());
    sort_definitions(result.definitions);
    sort_definitions(result.provenance);
    return result;
}

} // namespace oasis::tools
