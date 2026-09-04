#pragma once

#include "tools/re_atlas.hpp"
#include "tools/re_slice_decoder.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace oasis::tools {

enum class CfgAuditClassification {
    unreachable_code_candidate,
    embedded_data_candidate,
    secondary_entry_candidate,
    decoder_artifact_candidate,
    boundary_window_tail,
    unknown,
};

struct AuditIncomingEdge {
    std::uint32_t source{};
    std::uint32_t target{};
    std::uint32_t source_function{};
    std::string kind;
};

struct AuditRecord {
    std::uint32_t instruction_address{};
    std::uint32_t byte_end{};
    std::uint32_t block_start{};
    std::uint16_t opcode{};
    std::string mnemonic;
    std::string decoder_status;
    std::vector<std::uint8_t> bytes;
    std::vector<MemoryReference> direct_memory_references;
    std::vector<UnresolvedMemoryReference> unresolved_memory_references;
    std::optional<std::uint32_t> nearest_preceding_reachable;
    std::optional<std::uint32_t> nearest_following_reachable;
    std::optional<std::uint32_t> nearest_preceding_reachable_block;
    std::optional<std::uint32_t> nearest_following_reachable_block;
    std::uint32_t preceding_distance{};
    std::uint32_t following_distance{};
    std::vector<AuditIncomingEdge> incoming_edges;
    std::vector<std::uint32_t> outgoing_targets;
    bool fallthrough_possible{};
    bool alignment_padding_pattern{};
    bool embedded_data_pattern{};
    bool decoder_supported{};
    CfgAuditClassification classification{CfgAuditClassification::unknown};
    std::string confidence;
    std::string reason;
};

struct AuditIsland {
    std::string id;
    std::uint32_t start{};
    std::uint32_t end{};
    std::size_t byte_count{};
    std::size_t instruction_count{};
    std::vector<std::uint32_t> record_addresses;
    std::vector<AuditIncomingEdge> incoming_edges;
    std::vector<std::uint32_t> outgoing_targets;
    std::optional<std::uint32_t> terminating_instruction;
    CfgAuditClassification classification{CfgAuditClassification::unknown};
    std::string confidence;
};

struct CfgAuditCount {
    std::string key;
    std::size_t records{};
    std::size_t bytes{};
};

struct CfgAuditFactorCount {
    std::string key;
    std::size_t records{};
};

struct CfgAuditReport {
    std::uint32_t target_entry{};
    std::uint32_t window_start{};
    std::uint32_t window_end{};
    std::size_t raw_static_evidence_records{};
    std::size_t outside_reachable_records{};
    std::size_t reachable_unresolved_after_resolution{};
    std::size_t nonreachable_unresolved{};
    std::size_t raw_unresolved_after_resolution{};
    std::size_t records_with_known_incoming_edges{};
    std::size_t records_without_known_incoming_edges{};
    std::size_t secondary_entry_candidates{};
    std::size_t suspected_data_or_artifact_records{};
    std::size_t unknown_remainder{};
    std::size_t atlas_unresolved_before{};
    std::size_t atlas_unresolved_after_audit{};
    std::size_t ranking_displacement_before{};
    std::size_t ranking_displacement_after_audit{};
    std::string beta_evidence;
    std::vector<CfgAuditCount> classification_counts;
    std::vector<CfgAuditFactorCount> reachability_factors;
    std::vector<AuditRecord> records;
    std::vector<AuditIsland> islands;
};

[[nodiscard]] CfgAuditReport audit_bounded_unreachable_cfg(
    const AtlasReport& atlas, std::span<const std::uint8_t> retail_rom);

[[nodiscard]] CfgAuditClassification classify_cfg_audit_record(const AuditRecord& record);
[[nodiscard]] std::vector<AuditIsland> group_cfg_audit_islands(std::span<const AuditRecord> records);

[[nodiscard]] std::string cfg_audit_classification_name(CfgAuditClassification classification);
[[nodiscard]] std::string cfg_audit_to_json(const CfgAuditReport& report);
[[nodiscard]] std::string cfg_audit_to_text(const CfgAuditReport& report);

} // namespace oasis::tools
