#pragma once

#include "tools/re_slice_decoder.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace oasis::tools {

inline constexpr std::uint32_t kReTraceFunctionEntry = 0x0000A6A4;
inline constexpr std::uint32_t kReTraceStart = 0x0000A7D4;
inline constexpr std::uint32_t kReTraceCallbackReturn = 0x0000A7E4;

struct TraceRegisterSnapshot {
    std::optional<std::uint32_t> a0;
    std::optional<std::uint32_t> a6;
};

struct ExecutedInstruction {
    std::size_t step{};
    std::uint32_t address{};
    std::uint16_t opcode{};
    std::uint32_t block_start{};
    std::optional<TraceRegisterSnapshot> registers;
};

struct TraceBranchOutcome {
    std::uint32_t instruction_address{};
    std::uint32_t block_start{};
    std::uint32_t target{};
    bool taken{};
};

struct TraceCall {
    std::uint32_t instruction_address{};
    std::uint32_t block_start{};
    std::uint32_t target{};
    FlowKind kind{FlowKind::direct_call};
};

struct TraceReturn {
    std::uint32_t instruction_address{};
    std::uint32_t block_start{};
};

struct TraceMemoryAccess {
    std::uint32_t instruction_address{};
    std::uint32_t block_start{};
    std::uint32_t address{};
    std::uint8_t width_bytes{};
    MemoryAccess access{MemoryAccess::unknown};
    std::uint32_t value{};
    std::optional<TraceRegisterSnapshot> registers;
};

struct TraceIndirectTarget {
    std::uint32_t instruction_address{};
    std::uint32_t block_start{};
    std::uint32_t target{};
    std::optional<TraceRegisterSnapshot> registers;
};

struct StaticEvidence {
    std::uint32_t instruction_address{};
    std::uint32_t block_start{};
    std::string kind;
    std::optional<std::uint32_t> address;
    std::string reason;
};

struct DynamicEvidence {
    std::uint32_t instruction_address{};
    std::uint32_t block_start{};
    std::string kind;
    std::uint32_t observed_value{};
    std::uint8_t width_bytes{};
};

struct NewlyResolvedEvidence {
    std::uint32_t instruction_address{};
    std::uint32_t block_start{};
    std::string static_kind;
    std::uint32_t observed_value{};
    std::uint8_t width_bytes{};
};

struct TraceOptions {
    std::uint32_t function_entry{kReTraceFunctionEntry};
    std::size_t static_byte_budget{0x180};
    std::uint32_t start_pc{kReTraceStart};
    std::size_t instruction_budget{32};
    std::uint32_t initial_a6{0x00FF2954};
    std::uint16_t initial_record_word{1};
    std::uint32_t callback_target{kReTraceCallbackReturn};
};

struct TraceReport {
    std::uint32_t function_entry{};
    std::uint32_t start_pc{};
    std::size_t instruction_budget{};
    DecodedSlice static_slice;
    std::vector<ExecutedInstruction> executed_instructions;
    std::vector<std::uint32_t> executed_basic_blocks;
    std::vector<TraceBranchOutcome> branches;
    std::vector<TraceCall> calls;
    std::vector<TraceReturn> returns;
    std::vector<TraceMemoryAccess> memory_reads;
    std::vector<TraceMemoryAccess> memory_writes;
    std::vector<TraceIndirectTarget> indirect_targets;
    std::vector<StaticEvidence> static_confirmed;
    std::vector<DynamicEvidence> dynamic_observed;
    std::vector<NewlyResolvedEvidence> newly_resolved;
    std::vector<StaticEvidence> still_unresolved;
    std::string stop_reason;
};

[[nodiscard]] TraceReport trace_m68k_scenario(
    std::span<const std::uint8_t> rom, const TraceOptions& options = {});

[[nodiscard]] std::string trace_to_json(const TraceReport& report);
[[nodiscard]] std::string trace_to_text(const TraceReport& report);

} // namespace oasis::tools
