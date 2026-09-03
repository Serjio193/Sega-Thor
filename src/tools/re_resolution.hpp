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

enum class ResolutionStatus {
    resolved,
    unresolved_unknown_base,
    unresolved_conflicting_base,
    unresolved_unsupported_transfer,
    unresolved_cfg_merge,
};

enum class EffectiveAddressClass { rom, ram, outside_known_address_space, unknown };

struct ResolutionProofStep {
    std::uint32_t instruction_address{};
    std::uint32_t block_start{};
    std::string operation;
    std::uint32_t value{};
};

struct ResolutionItem {
    std::uint32_t instruction_address{};
    std::uint32_t block_start{};
    std::uint8_t base_register{};
    std::int16_t displacement{};
    ResolutionStatus status{ResolutionStatus::unresolved_unknown_base};
    EffectiveAddressClass address_class{EffectiveAddressClass::unknown};
    std::optional<std::uint32_t> base_value;
    std::optional<std::uint32_t> effective_address;
    std::string reason;
    std::vector<ResolutionProofStep> provenance;
};

struct ResolutionCount {
    std::string key;
    std::size_t count{};
};

struct ResolutionAddressRange {
    EffectiveAddressClass address_class{EffectiveAddressClass::unknown};
    std::uint32_t start{};
    std::uint32_t end{};
    std::size_t count{};
};

struct ResolutionRankingDelta {
    std::string dimension;
    std::string key;
    std::size_t before{};
    std::size_t after{};
    std::int64_t delta{};
};

struct ResolutionReport {
    std::uint32_t target_entry{};
    std::size_t static_candidate_count{};
    std::size_t newly_resolved{};
    std::size_t still_unresolved{};
    std::size_t provenance_failures{};
    std::size_t rom_effective_address_count{};
    std::size_t ram_effective_address_count{};
    std::size_t unique_concrete_address_count{};
    std::size_t atlas_unresolved_before{};
    std::size_t atlas_unresolved_after{};
    std::vector<ResolutionItem> items;
    std::vector<ResolutionCount> reason_counts;
    std::vector<ResolutionCount> base_register_counts;
    std::vector<ResolutionAddressRange> effective_address_ranges;
    std::vector<ResolutionRankingDelta> ranking_delta;
};

[[nodiscard]] ResolutionReport resolve_decoded_displacements(
    const AtlasEntry& atlas_entry, const DecodedSlice& slice);

[[nodiscard]] ResolutionReport resolve_bounded_displacements(
    const AtlasReport& atlas, std::span<const std::uint8_t> retail_rom);

[[nodiscard]] std::string resolution_status_name(ResolutionStatus status);
[[nodiscard]] std::string effective_address_class_name(EffectiveAddressClass address_class);
[[nodiscard]] std::string resolution_to_json(const ResolutionReport& report);
[[nodiscard]] std::string resolution_to_text(const ResolutionReport& report);

} // namespace oasis::tools
