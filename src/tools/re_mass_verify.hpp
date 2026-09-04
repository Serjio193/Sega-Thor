#pragma once

#include "tools/re_candidate_map.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace oasis::tools {

enum class MassBoundaryStatus { agrees, shorter, longer, unknown, conflict };
enum class StructuralClassification {
    strong_static, moderate_static, weak_static, indirect_flow, unsupported,
    boundary_conflict, data_conflict, decode_failure, unknown
};

struct MassCandidate {
    std::uint32_t entry{};
    CandidateClassification previous_classification{CandidateClassification::ghidra_only};
    CandidateComplexity complexity{CandidateComplexity::unknown};
    bool decode_ok{};
    bool first_instruction_supported{};
    bool direct_bsr_target{};
    bool direct_jsr_target{};
    bool known_static_target{};
    bool vector_target{};
    std::size_t direct_caller_count{};
    std::size_t reachable_instruction_count{};
    std::size_t reachable_block_count{};
    bool reaches_rts{};
    bool reaches_rte{};
    bool ends_known_direct_transfer{};
    bool unresolved_indirect_flow{};
    std::size_t unsupported_opcode_count{};
    std::size_t unsupported_addressing_count{};
    bool decode_conflict{};
    MassBoundaryStatus boundary_status{MassBoundaryStatus::unknown};
    bool known_data_overlap{};
    bool confirmed_code_overlap{};
    bool other_ghidra_overlap{};
    bool existing_beta_support{};
    bool existing_dynamic_support{};
    StructuralClassification structural_classification{StructuralClassification::unknown};
    std::vector<std::string> failure_reasons;
};

struct FailureCluster {
    std::string reason;
    std::size_t count{};
    std::vector<std::uint32_t> sample_addresses;
    bool systemic_fix_candidate{};
};

struct LeafBreakdown {
    std::size_t total{};
    std::size_t clean{};
    std::size_t unsupported{};
    std::size_t indirect{};
    std::size_t boundary_conflict{};
    std::size_t terminal_failure{};
    std::size_t other{};
};

struct ControlResult {
    std::uint32_t entry{};
    bool present{};
    bool heuristic_pass{};
    bool heuristic_miss{};
    StructuralClassification structural_classification{StructuralClassification::unknown};
};

struct SystemicFix {
    std::string blocker_class;
    std::size_t affected_count{};
    std::string effort;
    std::string risk;
    std::string expected_gain;
};

struct MassVerificationReport {
    std::string schema{"oasis.m68k.re-mass-verify.v1"};
    std::string candidate_map_schema;
    std::string ghidra_schema;
    std::string ghidra_program;
    std::size_t total_candidates{};
    std::vector<MassCandidate> candidates;
    std::vector<FailureCluster> failure_clusters;
    LeafBreakdown leaf;
    std::vector<ControlResult> control_set;
    std::vector<SystemicFix> top_systemic_fixes;
};

[[nodiscard]] MassVerificationReport verify_mass_structure(
    std::span<const std::uint8_t> rom, const CandidateMapReport& candidate_map,
    const AtlasReport& atlas);
[[nodiscard]] std::string mass_boundary_name(MassBoundaryStatus status);
[[nodiscard]] std::string structural_classification_name(StructuralClassification value);
[[nodiscard]] std::string mass_verify_to_json(const MassVerificationReport& report);
[[nodiscard]] std::string mass_verify_to_text(const MassVerificationReport& report);

} // namespace oasis::tools
