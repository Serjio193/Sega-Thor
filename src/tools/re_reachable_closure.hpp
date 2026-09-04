#pragma once

#include "tools/re_resolution.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace oasis::tools {

enum class ClosureReason {
    unknown_base,
    conflicting_cfg_merge,
    unsupported_transfer,
    call_clobber,
    entry_state_unknown,
    other,
};

struct ClosureDefinition {
    std::uint32_t instruction_address{};
    std::uint32_t block_start{};
    std::string operation;
    std::optional<std::uint32_t> value;
    bool supported{};
};

struct BackwardAnalysis {
    ClosureReason reason{ClosureReason::other};
    std::optional<std::uint32_t> value;
    std::vector<ClosureDefinition> definitions;
    std::vector<ResolutionProofStep> provenance;
    std::string stack_status;
    std::optional<std::uint32_t> a7_before;
    std::optional<std::uint32_t> a7_increment_bytes;
    std::vector<ClosureDefinition> stack_provenance;
};

struct ReachableClosureItem {
    std::uint32_t instruction_address{};
    std::uint32_t block_start{};
    std::uint16_t opcode{};
    std::vector<std::uint8_t> bytes;
    std::string mnemonic;
    std::vector<std::string> addressing_modes;
    std::string operand;
    std::uint8_t base_register{};
    std::int16_t displacement{};
    std::vector<std::uint32_t> cfg_predecessors;
    ResolutionStatus initial_status{ResolutionStatus::unresolved_unknown_base};
    std::string current_unresolved_reason;
    std::string prior_closure_reason;
    ClosureReason reason{ClosureReason::other};
    std::vector<ClosureDefinition> last_known_definitions;
    std::optional<ResolutionProofStep> nearest_proven_register_state;
    std::optional<std::uint32_t> effective_address;
    EffectiveAddressClass address_class{EffectiveAddressClass::unknown};
    std::vector<ResolutionProofStep> provenance;
    std::string stack_status;
    std::optional<std::uint32_t> a7_before;
    std::optional<std::uint32_t> a7_increment_bytes;
    std::vector<ClosureDefinition> stack_provenance;
    std::string evidence;
    std::string confidence;
};

struct ClosureReasonCount {
    std::string key;
    std::size_t count{};
};

struct ReachableClosureReport {
    std::uint32_t target_entry{};
    std::uint32_t window_start{};
    std::uint32_t window_end{};
    std::size_t exact_reachable_unresolved_count{};
    std::size_t raw_static_unresolved{};
    std::size_t raw_displacement_backlog{};
    std::size_t reachable_unresolved_before{};
    std::size_t newly_resolved{};
    std::size_t reachable_unresolved_after{};
    std::size_t nonreachable_unresolved{};
    std::size_t speculative_resolutions{};
    std::size_t provenance_failures{};
    std::size_t ram_effective_address_count{};
    std::size_t rom_effective_address_count{};
    std::size_t atlas_unresolved_before{};
    std::size_t atlas_unresolved_after{};
    std::size_t ranking_displacement_before{};
    std::size_t ranking_displacement_after{};
    std::string transfer_rule;
    std::string dynamic_scenario;
    std::vector<ClosureReasonCount> reason_counts;
    std::vector<ReachableClosureItem> items;
};

[[nodiscard]] ReachableClosureReport audit_reachable_unresolved(
    const AtlasReport& atlas, std::span<const std::uint8_t> retail_rom);

[[nodiscard]] ReachableClosureReport audit_reachable_unresolved(
    const AtlasEntry& atlas_entry, const DecodedSlice& slice);

[[nodiscard]] BackwardAnalysis analyze_bounded_backward_register(
    const DecodedSlice& slice, std::uint32_t instruction_address, std::uint8_t base_register);

[[nodiscard]] std::string closure_reason_name(ClosureReason reason);
[[nodiscard]] std::string reachable_closure_to_json(const ReachableClosureReport& report);
[[nodiscard]] std::string reachable_closure_to_text(const ReachableClosureReport& report);

} // namespace oasis::tools
