#pragma once

#include "tools/re_atlas.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace oasis::tools {

struct GhidraFunction {
    std::uint32_t entry{};
    std::optional<std::uint32_t> range_start;
    std::optional<std::uint32_t> range_end;
    std::vector<std::uint32_t> calls;
    std::vector<std::uint32_t> called_by;
    bool has_return{};
    std::size_t instruction_count{};
    std::size_t basic_block_count{};
};

struct GhidraCandidate {
    std::uint32_t entry{};
    std::optional<std::uint32_t> range_start;
    std::optional<std::uint32_t> range_end;
    std::string source;
    bool decoded_as_code{};
};

struct GhidraMap {
    std::string schema;
    std::string program_name;
    std::string language_id;
    std::string compiler_spec;
    std::string analysis_mode;
    std::string semantic_status;
    std::vector<GhidraFunction> functions;
    std::vector<GhidraCandidate> candidates;
};

enum class CandidateClassification { confirmed, static_supported, dynamic_observed,
                                     ghidra_only, conflict };
enum class CandidateComplexity { leaf, shallow, complex, unknown };

struct CandidateRecord {
    std::uint32_t entry{};
    std::vector<std::string> source_flags;
    bool ghidra_function{};
    std::optional<std::uint32_t> ghidra_range_start;
    std::optional<std::uint32_t> ghidra_range_end;
    std::vector<std::uint32_t> ghidra_calls;
    std::vector<std::uint32_t> ghidra_called_by;
    bool atlas_present{};
    std::string atlas_status;
    bool dynamic_observed{};
    std::vector<std::string> dynamic_sources;
    std::string beta_match_kind{"unknown"};
    std::optional<std::uint32_t> beta_address;
    bool known_direct_call_target{};
    bool known_direct_caller{};
    std::vector<std::uint32_t> known_direct_call_sites;
    bool code_data_conflict{};
    bool boundary_conflict{};
    bool indirect_flow{};
    CandidateComplexity complexity{CandidateComplexity::unknown};
    CandidateClassification classification{CandidateClassification::ghidra_only};
    int ranking_score{};
    std::vector<std::string> ranking_reasons;
    std::string quality_audit;
};

struct CandidateMapReport {
    std::string schema{"oasis.m68k.re-candidate-map.v1"};
    std::string ghidra_schema;
    std::string ghidra_program;
    std::string ghidra_language;
    std::string ghidra_analysis_mode;
    std::string ghidra_semantic_status;
    std::vector<CandidateRecord> candidates;
    std::size_t confirmed_count{};
    std::size_t static_supported_count{};
    std::size_t dynamic_observed_count{};
    std::size_t ghidra_only_count{};
    std::size_t conflict_count{};
    std::size_t leaf_count{};
    std::size_t shallow_count{};
    std::size_t complex_count{};
    std::size_t unknown_complexity_count{};
    std::optional<std::uint32_t> recommended_next_target;
    std::string recommended_reason;
};

[[nodiscard]] GhidraMap parse_ghidra_map(std::string_view text);
[[nodiscard]] CandidateMapReport build_candidate_map(
    const GhidraMap& ghidra, const AtlasReport& atlas);
[[nodiscard]] std::string candidate_map_to_json(const CandidateMapReport& report);
[[nodiscard]] std::string candidate_map_to_text(const CandidateMapReport& report,
                                                 std::size_t top_n = 20);
[[nodiscard]] std::string candidate_classification_name(CandidateClassification value);
[[nodiscard]] std::string candidate_complexity_name(CandidateComplexity value);

} // namespace oasis::tools
