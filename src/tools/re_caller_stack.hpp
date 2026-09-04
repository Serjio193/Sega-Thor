#pragma once

#include "tools/re_callee_effect.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace oasis::tools {

struct CallerStackValue {
    std::string expression;
    std::string kind{"unknown"};
    std::uint32_t source_instruction{};
};

struct CallerStackEvent {
    std::uint32_t instruction_address{};
    std::uint32_t block_start{};
    std::string event;
    std::string sp_before;
    std::string sp_after;
    std::string value;
};

struct CallerStackPath {
    std::vector<std::uint32_t> block_starts;
    std::vector<CallerStackEvent> events;
    std::optional<int> pre_call_sp_offset;
    std::optional<CallerStackValue> value_at_pre_call_sp;
    std::string status;
    std::vector<std::string> blockers;
};

struct CallerStackTargetResult {
    std::uint32_t instruction_address{};
    std::string prior_reason;
    std::string symbolic_a7_before;
    std::string stack_value;
    std::string value_kind{"unknown"};
    std::optional<std::uint32_t> effective_address;
    std::string address_class{"unknown"};
    std::string provenance;
    std::string status;
};

struct CallerStackReport {
    std::uint32_t entry{};
    std::uint32_t call_site{};
    std::uint32_t callee{};
    std::uint32_t window_start{};
    std::uint32_t window_end{};
    std::string symbolic_entry_sp{"S"};
    std::string symbolic_pre_call_sp{"UNKNOWN"};
    std::vector<std::uint32_t> containing_blocks;
    std::vector<std::uint32_t> predecessor_blocks;
    std::vector<CallerStackEvent> stack_events;
    std::vector<CallerStackPath> cfg_paths;
    std::string merge_status{"UNKNOWN"};
    std::optional<CallerStackValue> value_at_pre_call_sp;
    std::string value_kind{"unknown"};
    std::vector<std::string> provenance;
    std::vector<std::string> blockers;
    std::vector<CallerStackTargetResult> target_results;
    std::size_t relevant_path_count{};
    std::size_t stack_event_count{};
    std::size_t prior_calls_crossed{};
    std::size_t known_call_effects{};
    std::size_t unknown_call_effects{};
    std::size_t stack_merge_conflicts{};
    std::size_t reachable_unresolved_before{16};
    std::size_t reachable_unresolved_after{16};
    std::size_t speculative_resolutions{};
};

[[nodiscard]] CallerStackReport analyze_caller_stack(
    const DecodedSlice& caller_slice, const CalleeEffectReport& known_callee);

[[nodiscard]] CallerStackReport audit_caller_stack(std::span<const std::uint8_t> retail_rom);

[[nodiscard]] std::string caller_stack_to_json(const CallerStackReport& report);
[[nodiscard]] std::string caller_stack_to_text(const CallerStackReport& report);

} // namespace oasis::tools
