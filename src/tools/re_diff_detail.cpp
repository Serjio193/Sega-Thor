#include "tools/re_diff.hpp"

#include <algorithm>
#include <map>
#include <sstream>

namespace oasis::tools {
namespace {

const DecodedInstruction* instruction_at(const DecodedSlice& slice, std::uint32_t address) {
    const auto item = std::find_if(slice.instructions.begin(), slice.instructions.end(),
        [address](const auto& instruction) { return instruction.address == address; });
    return item == slice.instructions.end() ? nullptr : &*item;
}

std::map<std::uint32_t, std::size_t> block_indexes(const DecodedSlice& slice) {
    std::map<std::uint32_t, std::size_t> result;
    for (std::size_t index = 0; index < slice.basic_blocks.size(); ++index) {
        for (const auto address : slice.basic_blocks[index].instruction_addresses) result[address] = index;
    }
    return result;
}

std::string block_target_shape(const DecodedSlice& slice, const ControlFlowEdge& edge) {
    const auto indexes = block_indexes(slice);
    const auto source = indexes.find(edge.source);
    const auto target = std::find_if(slice.basic_blocks.begin(), slice.basic_blocks.end(),
        [&edge](const auto& block) {
            return std::find(block.instruction_addresses.begin(), block.instruction_addresses.end(), edge.target) !=
                   block.instruction_addresses.end();
        });
    std::ostringstream output;
    output << (source == indexes.end() ? "external" : std::to_string(source->second)) << ':'
           << flow_kind_name(edge.kind) << ':'
           << (target == slice.basic_blocks.end() ? "external" : std::to_string(
               static_cast<std::size_t>(target - slice.basic_blocks.begin())));
    return output.str();
}

std::vector<ControlFlowEdge> fallthrough_edges(const DecodedSlice& slice, const BasicBlock& block) {
    if (block.instruction_addresses.empty()) return {};
    const auto* last = instruction_at(slice, block.instruction_addresses.back());
    if (last == nullptr || last->flow != FlowKind::direct_branch || last->mnemonic == "bra") return {};
    return {{last->address, static_cast<std::uint32_t>(last->address + last->bytes.size()), FlowKind::none}};
}

std::vector<std::string> block_topology(const DecodedSlice& slice, const BasicBlock& block) {
    std::vector<std::string> result;
    for (const auto& edge : slice.control_flow) {
        if (std::find(block.instruction_addresses.begin(), block.instruction_addresses.end(), edge.source) !=
            block.instruction_addresses.end()) result.push_back(block_target_shape(slice, edge));
    }
    for (const auto& item : slice.unresolved_control_flow) {
        if (std::find(block.instruction_addresses.begin(), block.instruction_addresses.end(), item.address) !=
            block.instruction_addresses.end()) result.push_back("unresolved:" + flow_kind_name(item.kind));
    }
    for (const auto& edge : fallthrough_edges(slice, block)) result.push_back(block_target_shape(slice, edge));
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<ControlFlowEdge> predecessor_edges(const DecodedSlice& slice, const BasicBlock& block) {
    std::vector<ControlFlowEdge> result;
    for (const auto& edge : slice.control_flow) if (edge.target == block.start) result.push_back(edge);
    return result;
}

std::vector<ControlFlowEdge> successor_edges(const DecodedSlice& slice, const BasicBlock& block) {
    std::vector<ControlFlowEdge> result;
    for (const auto& edge : slice.control_flow) {
        if (std::find(block.instruction_addresses.begin(), block.instruction_addresses.end(), edge.source) !=
            block.instruction_addresses.end()) result.push_back(edge);
    }
    return result;
}

std::vector<ControlFlowEdge> fallthrough_predecessors(const DecodedSlice& slice, const BasicBlock& block) {
    std::vector<ControlFlowEdge> result;
    for (const auto& candidate : slice.basic_blocks) {
        for (const auto& edge : fallthrough_edges(slice, candidate)) if (edge.target == block.start) result.push_back(edge);
    }
    return result;
}

std::vector<UnresolvedControlFlow> unresolved_successors(const DecodedSlice& slice, const BasicBlock& block) {
    std::vector<UnresolvedControlFlow> result;
    for (const auto& item : slice.unresolved_control_flow) {
        if (std::find(block.instruction_addresses.begin(), block.instruction_addresses.end(), item.address) !=
            block.instruction_addresses.end()) result.push_back(item);
    }
    return result;
}

bool same_memory_shape(const DecodedInstruction& left, const DecodedInstruction& right) {
    if (left.memory_references.size() != right.memory_references.size()) return false;
    for (std::size_t index = 0; index < left.memory_references.size(); ++index) {
        const auto& a = left.memory_references[index];
        const auto& b = right.memory_references[index];
        if (a.width_bytes != b.width_bytes || a.kind != b.kind || a.access != b.access) return false;
    }
    return true;
}

bool same_immediate_shape(const DecodedInstruction& left, const DecodedInstruction& right) {
    if (left.immediate_constants.size() != right.immediate_constants.size()) return false;
    for (std::size_t index = 0; index < left.immediate_constants.size(); ++index) {
        if (left.immediate_constants[index].width_bytes != right.immediate_constants[index].width_bytes) return false;
    }
    return true;
}

bool immediate_values_differ(const DecodedInstruction& left, const DecodedInstruction& right) {
    if (left.immediate_constants.size() != right.immediate_constants.size()) return true;
    for (std::size_t index = 0; index < left.immediate_constants.size(); ++index) {
        if (left.immediate_constants[index].value != right.immediate_constants[index].value ||
            left.immediate_constants[index].width_bytes != right.immediate_constants[index].width_bytes) return true;
    }
    return false;
}

bool same_unresolved_memory(const DecodedInstruction& left, const DecodedInstruction& right) {
    if (left.unresolved_memory_references.size() != right.unresolved_memory_references.size()) return false;
    for (std::size_t index = 0; index < left.unresolved_memory_references.size(); ++index) {
        const auto& a = left.unresolved_memory_references[index];
        const auto& b = right.unresolved_memory_references[index];
        if (a.mode != b.mode || a.register_index != b.register_index || a.reason != b.reason) return false;
    }
    return true;
}

bool same_non_value_evidence(const DecodedInstruction& left, const DecodedInstruction& right) {
    return left.mnemonic == right.mnemonic && left.supported == right.supported && left.flow == right.flow &&
           left.addressing_modes == right.addressing_modes && same_immediate_shape(left, right) &&
           same_memory_shape(left, right) && same_unresolved_memory(left, right) &&
           left.unsupported_addressing.size() == right.unsupported_addressing.size();
}

bool immediate_is_relocated_instruction(const DecodedSlice& left_slice, const DecodedSlice& right_slice,
                                        const DecodedInstruction& left, const DecodedInstruction& right) {
    if (left.immediate_constants.size() != right.immediate_constants.size()) return false;
    for (std::size_t index = 0; index < left.immediate_constants.size(); ++index) {
        const auto left_value = left.immediate_constants[index].value;
        const auto right_value = right.immediate_constants[index].value;
        if (instruction_at(left_slice, left_value) == nullptr || instruction_at(right_slice, right_value) == nullptr ||
            left_value - left_slice.entry != right_value - right_slice.entry) return false;
    }
    return !left.immediate_constants.empty();
}

std::vector<InstructionDiffKind> classify_instruction(const DecodedSlice& left_slice,
                                                      const DecodedSlice& right_slice,
                                                      const DecodedInstruction& left,
                                                      const DecodedInstruction& right) {
    if (left.bytes == right.bytes) return {InstructionDiffKind::identical};
    std::vector<InstructionDiffKind> result;
    if (immediate_values_differ(left, right) && same_immediate_shape(left, right)) {
        result.push_back(immediate_is_relocated_instruction(left_slice, right_slice, left, right)
                             ? InstructionDiffKind::relocation_only : InstructionDiffKind::constant_changed);
    }
    const auto memory_addresses_changed = left.memory_references.size() == right.memory_references.size() &&
        std::any_of(left.memory_references.begin(), left.memory_references.end(),
            [&right, index = std::size_t{0}](const auto& reference) mutable {
                return reference.address != right.memory_references[index++].address;
            });
    if (memory_addresses_changed && same_memory_shape(left, right)) result.push_back(InstructionDiffKind::memory_offset_changed);
    if ((left.flow == FlowKind::direct_branch || left.flow == FlowKind::direct_jump || left.flow == FlowKind::direct_call ||
         right.flow == FlowKind::direct_branch || right.flow == FlowKind::direct_jump || right.flow == FlowKind::direct_call) &&
        (left.flow != right.flow || left.direct_target != right.direct_target)) result.push_back(InstructionDiffKind::branch_changed);
    if (left.addressing_modes != right.addressing_modes) result.push_back(InstructionDiffKind::addressing_mode_changed);
    if (result.empty() && same_non_value_evidence(left, right) &&
        (left.direct_target != right.direct_target || memory_addresses_changed)) result.push_back(InstructionDiffKind::relocation_only);
    if (result.empty() && (!left.supported || !right.supported || !left.unresolved_memory_references.empty() ||
                           !right.unresolved_memory_references.empty() || !left.unsupported_addressing.empty() ||
                           !right.unsupported_addressing.empty())) result.push_back(InstructionDiffKind::unresolved);
    if (result.empty()) result.push_back(InstructionDiffKind::unresolved);
    return result;
}

BlockDetail make_block_detail(const DecodedSlice& retail, const DecodedSlice& beta,
                              std::size_t ordinal, const ChangedBlock& changed) {
    BlockDetail detail{.ordinal = ordinal};
    if (changed.retail_start) {
        const auto block = std::find_if(retail.basic_blocks.begin(), retail.basic_blocks.end(),
            [start = *changed.retail_start](const auto& item) { return item.start == start; });
        if (block != retail.basic_blocks.end()) detail.retail_block = *block;
    }
    if (changed.beta_start) {
        const auto block = std::find_if(beta.basic_blocks.begin(), beta.basic_blocks.end(),
            [start = *changed.beta_start](const auto& item) { return item.start == start; });
        if (block != beta.basic_blocks.end()) detail.beta_block = *block;
    }
    if (detail.retail_block) {
        detail.retail_predecessors = predecessor_edges(retail, *detail.retail_block);
        detail.retail_fallthrough_predecessors = fallthrough_predecessors(retail, *detail.retail_block);
        detail.retail_successors = successor_edges(retail, *detail.retail_block);
        detail.retail_fallthrough_edges = fallthrough_edges(retail, *detail.retail_block);
        detail.retail_unresolved_successors = unresolved_successors(retail, *detail.retail_block);
    }
    if (detail.beta_block) {
        detail.beta_predecessors = predecessor_edges(beta, *detail.beta_block);
        detail.beta_fallthrough_predecessors = fallthrough_predecessors(beta, *detail.beta_block);
        detail.beta_successors = successor_edges(beta, *detail.beta_block);
        detail.beta_fallthrough_edges = fallthrough_edges(beta, *detail.beta_block);
        detail.beta_unresolved_successors = unresolved_successors(beta, *detail.beta_block);
    }
    const auto retail_topology = detail.retail_block ? block_topology(retail, *detail.retail_block) : std::vector<std::string>{"missing"};
    const auto beta_topology = detail.beta_block ? block_topology(beta, *detail.beta_block) : std::vector<std::string>{"missing"};
    if (retail_topology != beta_topology) detail.topology_differences = {"retail=" + (retail_topology.empty() ? "none" : retail_topology.front()),
                                                                           "beta=" + (beta_topology.empty() ? "none" : beta_topology.front())};
    const auto retail_instructions = detail.retail_block ? detail.retail_block->instruction_addresses : std::vector<std::uint32_t>{};
    const auto beta_instructions = detail.beta_block ? detail.beta_block->instruction_addresses : std::vector<std::uint32_t>{};
    const auto count = std::max(retail_instructions.size(), beta_instructions.size());
    for (std::size_t index = 0; index < count; ++index) {
        InstructionDifference difference;
        if (index < retail_instructions.size()) {
            difference.retail_address = retail_instructions[index];
            difference.retail_instruction = *instruction_at(retail, retail_instructions[index]);
        }
        if (index < beta_instructions.size()) {
            difference.beta_address = beta_instructions[index];
            difference.beta_instruction = *instruction_at(beta, beta_instructions[index]);
        }
        if (difference.retail_instruction && difference.beta_instruction) {
            difference.classifications = classify_instruction(retail, beta, *difference.retail_instruction,
                                                              *difference.beta_instruction);
        } else if (difference.retail_instruction) {
            difference.classifications = {InstructionDiffKind::instruction_removed};
        } else {
            difference.classifications = {InstructionDiffKind::instruction_added};
        }
        detail.instruction_differences.push_back(std::move(difference));
    }
    if (!detail.topology_differences.empty()) {
        for (auto& difference : detail.instruction_differences) difference.classifications.push_back(
            InstructionDiffKind::control_flow_topology_changed);
    }
    return detail;
}

} // namespace

std::vector<BlockDetail> make_changed_block_details(const DecodedSlice& retail, const DecodedSlice& beta,
                                                    const std::vector<ChangedBlock>& changed) {
    std::vector<BlockDetail> result;
    for (const auto& block : changed) result.push_back(make_block_detail(retail, beta, block.ordinal, block));
    return result;
}

} // namespace oasis::tools
