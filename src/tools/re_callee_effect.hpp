#pragma once

#include "tools/re_slice_decoder.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace oasis::tools {

struct CalleeRegisterEffect {
    std::uint8_t register_index{};
    std::string effect;
    std::optional<std::uint32_t> known_value;
    std::vector<std::uint32_t> evidence_instructions;
};

struct CalleeMemoryEvidence {
    std::uint32_t instruction_address{};
    std::uint32_t block_start{};
    std::optional<std::uint32_t> address;
    std::uint8_t width_bytes{};
    std::string access;
    std::string classification;
    std::string reason;
};

struct CalleeStackEffect {
    std::string entry_a7;
    std::string bsr_push;
    std::string internal_operations;
    std::string rts_pop;
    std::string net_after_return;
    std::string status;
};

struct CalleeTargetRecheck {
    std::uint32_t instruction_address{};
    std::string prior_reason;
    std::string provenance;
    std::string a7_after_call;
    std::string stack_value;
    std::optional<std::uint32_t> effective_address;
    std::string address_class;
    std::string status;
};

struct CalleeEffectReport {
    std::uint32_t requested_entry{};
    std::uint32_t call_site{};
    std::uint32_t entry{};
    std::uint32_t bounded_start{};
    std::uint32_t bounded_end{};
    bool boundary_proven{};
    std::string boundary_status;
    std::vector<std::uint32_t> reachable_blocks;
    std::vector<ControlFlowEdge> control_flow_edges;
    std::vector<std::uint32_t> return_sites;
    std::vector<CalleeRegisterEffect> register_effects;
    CalleeStackEffect stack_effect;
    std::vector<CalleeMemoryEvidence> memory_references;
    std::vector<CalleeMemoryEvidence> unresolved_memory_references;
    std::vector<std::uint32_t> direct_callees;
    std::vector<std::uint32_t> indirect_flow;
    std::vector<std::uint32_t> unsupported_instructions;
    std::vector<CalleeTargetRecheck> target_rechecks;
    std::vector<std::string> provenance;
};

[[nodiscard]] CalleeEffectReport analyze_callee_effect(
    const DecodedSlice& call_site_slice, const DecodedSlice& callee_slice);

[[nodiscard]] CalleeEffectReport audit_callee_effect(std::span<const std::uint8_t> retail_rom);

[[nodiscard]] std::string callee_effect_to_json(const CalleeEffectReport& report);
[[nodiscard]] std::string callee_effect_to_text(const CalleeEffectReport& report);

} // namespace oasis::tools
