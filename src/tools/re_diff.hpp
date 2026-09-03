#pragma once

#include "core/rom_identity.hpp"
#include "tools/re_program.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace oasis::tools {

enum class MatchKind { exact_match, structural_match, changed_blocks, unmatched };

struct DifferentialTarget {
    std::uint32_t entry{};
    std::size_t byte_budget{};
    std::optional<std::uint32_t> confirmed_end;
};

struct ChangedBlock {
    std::size_t ordinal{};
    std::optional<std::uint32_t> retail_start;
    std::optional<std::uint32_t> beta_start;
};

enum class InstructionDiffKind {
    identical,
    relocation_only,
    constant_changed,
    memory_offset_changed,
    branch_changed,
    instruction_added,
    instruction_removed,
    addressing_mode_changed,
    control_flow_topology_changed,
    unresolved,
};

struct InstructionDifference {
    std::optional<std::uint32_t> retail_address;
    std::optional<std::uint32_t> beta_address;
    std::vector<InstructionDiffKind> classifications;
    std::optional<DecodedInstruction> retail_instruction;
    std::optional<DecodedInstruction> beta_instruction;
};

struct BlockDetail {
    std::size_t ordinal{};
    std::optional<BasicBlock> retail_block;
    std::optional<BasicBlock> beta_block;
    std::vector<ControlFlowEdge> retail_predecessors;
    std::vector<ControlFlowEdge> beta_predecessors;
    std::vector<ControlFlowEdge> retail_fallthrough_predecessors;
    std::vector<ControlFlowEdge> beta_fallthrough_predecessors;
    std::vector<ControlFlowEdge> retail_successors;
    std::vector<ControlFlowEdge> beta_successors;
    std::vector<ControlFlowEdge> retail_fallthrough_edges;
    std::vector<ControlFlowEdge> beta_fallthrough_edges;
    std::vector<UnresolvedControlFlow> retail_unresolved_successors;
    std::vector<UnresolvedControlFlow> beta_unresolved_successors;
    std::vector<std::string> topology_differences;
    std::vector<InstructionDifference> instruction_differences;
};

struct AnalogCandidate {
    std::uint32_t beta_entry{};
    MatchKind match{MatchKind::unmatched};
    std::size_t matching_instructions{};
    std::size_t matching_blocks{};
    std::vector<ChangedBlock> changed_blocks;
    std::vector<BlockDetail> changed_block_details;
    std::vector<std::string> beta_normalized_opcode_signature;
};

struct TargetComparison {
    DifferentialTarget target;
    std::uint32_t retail_range_end{};
    std::uint32_t beta_same_address_range_end{};
    std::size_t retail_instructions{};
    std::size_t retail_basic_blocks{};
    std::vector<std::string> retail_normalized_opcode_signature;
    std::vector<std::string> beta_same_address_normalized_opcode_signature;
    MatchKind same_address_match{MatchKind::unmatched};
    std::vector<ChangedBlock> same_address_changed_blocks;
    std::vector<BlockDetail> same_address_changed_block_details;
    std::vector<AnalogCandidate> analogs;
    std::size_t unmatched_retail_instructions{};
    std::size_t unmatched_beta_instructions{};
};

struct DifferentialReport {
    RomIdentity retail;
    RomIdentity beta;
    std::vector<TargetComparison> targets;
};

[[nodiscard]] std::string match_kind_name(MatchKind kind);
[[nodiscard]] std::string instruction_diff_kind_name(InstructionDiffKind kind);
[[nodiscard]] std::vector<BlockDetail> make_changed_block_details(
    const DecodedSlice& retail, const DecodedSlice& beta,
    const std::vector<ChangedBlock>& changed);

[[nodiscard]] DifferentialReport compare_m68k_revisions(
    std::span<const std::uint8_t> retail_rom,
    std::span<const std::uint8_t> beta_rom,
    std::span<const DifferentialTarget> targets);

[[nodiscard]] std::string diff_to_json(const DifferentialReport& report);
[[nodiscard]] std::string diff_to_text(const DifferentialReport& report);

} // namespace oasis::tools
