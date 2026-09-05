#pragma once

#include "tools/re_atlas.hpp"
#include "tools/re_candidate_map.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace oasis::tools {

enum class SeedTier { tier0, tier1, tier2, tier3, tier4 };
enum class RegionType { unknown, instruction, known_data, probable_data, pointer_data, conflict };
enum class AnalysisState {
    unseen, discovered, queued, analyzing, analyzed, blocked_indirect,
    blocked_unsupported, blocked_data, conflict
};
enum class ExploreEdgeKind {
    direct_call, direct_jump, conditional_branch, dbcc, fallthrough, unresolved_indirect,
    dynamic_indirect
};
enum class StopReason {
    return_instruction, direct_terminal_transfer, indirect_transfer,
    unsupported_instruction, known_data, out_of_rom, conflict,
    already_analyzed, decode_failure
};
enum class FrontierType {
    indirect_flow, unsupported, decode_failure, boundary_conflict,
    code_data_conflict, out_of_rom, other_unknown
};

struct ExploreSeed {
    std::uint32_t address{};
    SeedTier tier{SeedTier::tier4};
    std::uint32_t priority{};
    std::string source;
    std::string source_reference;
    std::string initial_confidence;
};

struct ExploreEdge {
    std::uint32_t source_entry{};
    std::uint32_t source_pc{};
    std::uint32_t target{};
    ExploreEdgeKind kind{ExploreEdgeKind::fallthrough};
    bool target_queued{};
    std::string evidence_class{"STATIC_PROVEN"};
    std::string frontier_id;
    std::string job_id;
    std::string result_hash;
    std::string backend;
    std::string scenario;
};

struct DynamicEdgeEvidence {
    std::string frontier_id;
    std::uint32_t source_entry{};
    std::uint32_t source_pc{};
    std::uint32_t target{};
    std::string evidence_class{"DYNAMIC_NATURAL"};
    std::string job_id;
    std::string result_hash;
    std::string backend;
    std::string scenario;
};

struct PathTermination {
    std::uint32_t source_entry{};
    std::uint32_t source_pc{};
    StopReason reason{StopReason::decode_failure};
    std::string detail;
};

struct FrontierRecord {
    std::string id;
    std::uint32_t source_entry{};
    std::uint32_t source_pc{};
    FrontierType blocker_type{FrontierType::other_unknown};
    std::vector<std::uint8_t> instruction_bytes;
    std::uint16_t opcode{};
    std::string instruction;
    std::string known_context;
    std::vector<std::uint32_t> owners;
    std::vector<std::string> evidence;
    StopReason stop_reason{StopReason::decode_failure};
    std::string reason;
};

struct ExploreMapRange {
    std::uint32_t start{};
    std::uint32_t end{};
    RegionType type{RegionType::unknown};
    std::vector<std::uint32_t> owners;
    std::vector<std::string> sources;
    std::string confidence;
    std::string evidence;
    AnalysisState status{AnalysisState::unseen};
};

struct ExploreEntry {
    std::uint32_t address{};
    AnalysisState state{AnalysisState::unseen};
    std::vector<ExploreSeed> seeds;
    std::size_t decoded_instruction_count{};
    std::size_t supported_instruction_bytes{};
    std::size_t block_count{};
    std::size_t frontier_count{};
    std::size_t stop_count{};
};

struct ControlExpectation {
    std::uint32_t source{};
    std::uint32_t target{};
};

struct ControlResult {
    std::uint32_t entry{};
    bool present{};
    bool analyzed{};
    bool heuristic_pass{};
    bool heuristic_miss{};
    std::vector<ControlExpectation> recovered_edges;
};

struct BlockerCluster {
    std::string blocker_type;
    std::size_t count{};
    std::vector<std::uint32_t> representative_addresses;
};

struct ExploreMetrics {
    std::size_t total_rom_bytes{};
    std::size_t instruction_bytes{};
    std::size_t known_data_bytes{};
    std::size_t probable_data_bytes{};
    std::size_t pointer_data_bytes{};
    std::size_t conflict_bytes{};
    std::size_t unclassified_bytes{};
    std::size_t seeds{};
    std::size_t discovered_entries{};
    std::size_t queued_entries{};
    std::size_t analyzed_entries{};
    std::size_t blocked_indirect_entries{};
    std::size_t blocked_unsupported_entries{};
    std::size_t blocked_data_entries{};
    std::size_t conflict_entries{};
    std::size_t direct_calls{};
    std::size_t direct_jumps{};
    std::size_t conditional_branches{};
    std::size_t fallthroughs{};
    std::size_t unresolved_indirect{};
    std::size_t dynamic_indirect_edges{};
    std::size_t decoded_instructions{};
    std::size_t unsupported_instructions{};
    std::size_t decode_failures{};
    std::size_t frontier_count{};
    std::size_t entries_processed{};
};

struct ExploreOptions {
    bool rom_wide{};
    std::size_t entry_byte_budget{0x1000};
    std::size_t instruction_budget{4096};
    std::size_t max_entries{5000};
    std::vector<std::uint32_t> control_entries;
    std::vector<ControlExpectation> control_edges;
    std::vector<DynamicEdgeEvidence> dynamic_edges;
};

struct ExploreReport {
    std::string schema{"oasis.m68k.re-explore.v1"};
    std::string rom_identity;
    bool bounded_control_pass{};
    bool rom_wide_requested{};
    bool rom_wide_performed{};
    std::string rom_wide_skip_reason;
    std::vector<ExploreSeed> seeds;
    std::vector<std::uint32_t> processing_order;
    std::vector<ExploreEntry> entries;
    std::vector<ExploreEdge> edges;
    std::vector<PathTermination> stops;
    std::vector<FrontierRecord> frontier;
    std::vector<ExploreMapRange> address_map;
    std::vector<ControlResult> control_set;
    std::vector<BlockerCluster> blocker_clusters;
    ExploreMetrics metrics;
};

[[nodiscard]] ExploreReport explore_m68k(
    std::span<const std::uint8_t> rom, const CandidateMapReport& candidates,
    const AtlasReport& atlas, const ExploreOptions& options = {});

[[nodiscard]] std::string seed_tier_name(SeedTier value);
[[nodiscard]] std::string region_type_name(RegionType value);
[[nodiscard]] std::string analysis_state_name(AnalysisState value);
[[nodiscard]] std::string explore_edge_name(ExploreEdgeKind value);
[[nodiscard]] std::string stop_reason_name(StopReason value);
[[nodiscard]] std::string frontier_type_name(FrontierType value);
[[nodiscard]] const DynamicEdgeEvidence* find_dynamic_edge(
    std::span<const DynamicEdgeEvidence> edges, std::uint32_t source_entry,
    std::uint32_t source_pc);
[[nodiscard]] std::string explore_to_json(const ExploreReport& report);
[[nodiscard]] std::string explore_to_text(const ExploreReport& report);

} // namespace oasis::tools
