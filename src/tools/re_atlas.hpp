#pragma once

#include "core/rom_identity.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace oasis::tools {

enum class AtlasEntryType { function, bounded_code, data, table, unknown_ref };
enum class AtlasConfidence { confirmed, likely, hypothesis, unknown };
enum class AtlasCorrespondence { exact, structural, changed, unmatched, not_checked };
enum class NativeStatus { verified, present_unverified, not_applicable, not_implemented };

struct AtlasBetaCorrespondence {
    std::uint32_t address{};
    AtlasCorrespondence match{AtlasCorrespondence::not_checked};
    std::vector<std::size_t> changed_blocks;
};

struct AtlasDynamicSummary {
    std::size_t executed_instructions{};
    std::size_t executed_basic_blocks{};
    std::size_t memory_reads{};
    std::size_t memory_writes{};
    std::size_t branches{};
    std::size_t calls{};
    std::size_t returns{};
    std::vector<std::string> raw_facts;
};

struct AtlasNativeImplementation {
    NativeStatus status{NativeStatus::not_implemented};
    std::optional<std::string> path;
};

struct AtlasUnresolvedReference {
    std::uint32_t instruction_address{};
    std::uint32_t block_start{};
    std::uint8_t mode{};
    std::uint8_t register_index{};
    std::string addressing_mode;
    std::string instruction_family;
    std::string reason;
    bool dynamic_resolvable_candidate{};
    bool constant_propagation_candidate{};
};

struct AtlasUnsupportedEvidence {
    std::uint32_t instruction_address{};
    std::uint32_t block_start{};
    std::string kind;
    std::string reason;
};

struct AtlasEntry {
    std::string id;
    AtlasEntryType type{AtlasEntryType::unknown_ref};
    std::uint32_t start{};
    std::optional<std::uint32_t> end;
    std::optional<std::uint32_t> bounded_evidence_end;
    AtlasConfidence semantic_confidence{AtlasConfidence::unknown};
    AtlasConfidence boundary_confidence{AtlasConfidence::unknown};
    std::vector<std::string> evidence_sources;
    std::vector<std::uint32_t> callers;
    std::vector<std::uint32_t> callees;
    std::vector<std::uint32_t> direct_rom_refs;
    std::vector<std::uint32_t> direct_ram_refs;
    std::vector<AtlasUnresolvedReference> unresolved_references;
    std::vector<AtlasUnsupportedEvidence> unsupported_evidence;
    std::size_t unresolved_reference_count{};
    std::size_t unsupported_evidence_count{};
    std::size_t indirect_control_flow_count{};
    std::optional<AtlasBetaCorrespondence> beta;
    std::optional<AtlasDynamicSummary> dynamic;
    AtlasNativeImplementation native;
    std::string verification_status;
    std::string notes;
};

struct AtlasCallEdge {
    std::uint32_t caller{};
    std::uint32_t callee{};
    std::vector<std::uint32_t> call_sites;
};

struct AtlasConflict {
    std::string left_id;
    std::string right_id;
    std::uint32_t overlap_start{};
    std::uint32_t overlap_end{};
    std::string reason;
};

struct AtlasCoverage {
    std::size_t rom_size{};
    std::size_t confirmed_classified_bytes{};
    std::size_t bounded_evidence_bytes{};
    std::size_t overlapping_evidence_bytes{};
    std::size_t unknown_remainder_bytes{};
    std::size_t atlas_entries{};
    std::size_t verified_native_implementations{};
    std::size_t unresolved_unknown_references{};
};

struct AtlasReport {
    RomIdentity retail;
    std::optional<RomIdentity> beta;
    std::vector<AtlasEntry> entries;
    std::vector<AtlasCallEdge> call_edges;
    std::vector<AtlasConflict> conflicts;
    AtlasCoverage coverage;
};

[[nodiscard]] AtlasReport build_rom_atlas(
    std::span<const std::uint8_t> retail_rom,
    std::optional<std::span<const std::uint8_t>> beta_rom = std::nullopt);

[[nodiscard]] std::vector<AtlasConflict> detect_atlas_conflicts(
    std::span<const AtlasEntry> entries);

[[nodiscard]] std::vector<const AtlasEntry*> atlas_entries_at(
    const AtlasReport& report, std::uint32_t address);
[[nodiscard]] std::vector<std::uint32_t> atlas_callers(
    const AtlasReport& report, std::uint32_t entry);
[[nodiscard]] std::vector<std::uint32_t> atlas_callees(
    const AtlasReport& report, std::uint32_t entry);
[[nodiscard]] std::vector<const AtlasEntry*> atlas_refs_to_rom(
    const AtlasReport& report, std::uint32_t address);
[[nodiscard]] std::vector<const AtlasEntry*> atlas_refs_to_ram(
    const AtlasReport& report, std::uint32_t address);
[[nodiscard]] std::vector<const AtlasEntry*> atlas_entries_without_native(
    const AtlasReport& report);
[[nodiscard]] std::vector<const AtlasEntry*> atlas_entries_with_unresolved(
    const AtlasReport& report);
[[nodiscard]] std::optional<AtlasBetaCorrespondence> atlas_beta_lookup(
    const AtlasReport& report, std::uint32_t entry);

[[nodiscard]] std::string atlas_entry_type_name(AtlasEntryType type);
[[nodiscard]] std::string atlas_confidence_name(AtlasConfidence confidence);
[[nodiscard]] std::string atlas_correspondence_name(AtlasCorrespondence match);
[[nodiscard]] std::string native_status_name(NativeStatus status);
[[nodiscard]] std::string atlas_to_json(const AtlasReport& report);
[[nodiscard]] std::string atlas_to_text(const AtlasReport& report);

} // namespace oasis::tools
