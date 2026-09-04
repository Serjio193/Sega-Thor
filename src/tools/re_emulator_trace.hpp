#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace oasis::tools {

enum class EmulatorEventKind { instruction, branch, call, return_instruction, memory_read, memory_write, indirect_control_flow };

struct EmulatorRegisterSnapshot {
    std::array<std::optional<std::uint32_t>, 8> d;
    std::array<std::optional<std::uint32_t>, 8> a;
    std::optional<std::uint16_t> sr;
};

struct ExternalTraceEvent {
    std::uint64_t sequence{};
    std::optional<std::uint64_t> frame;
    std::optional<std::uint64_t> cycles;
    std::uint32_t pc{};
    EmulatorEventKind kind{EmulatorEventKind::instruction};
    std::optional<std::uint32_t> block_start;
    std::optional<std::uint32_t> target;
    std::optional<bool> taken;
    std::optional<std::uint32_t> address;
    std::uint8_t width_bytes{};
    std::uint8_t instruction_size{};
    std::optional<EmulatorRegisterSnapshot> registers;
};

struct ExternalTraceCapture {
    std::string emulator;
    std::string backend;
    std::string version;
    std::string scenario;
    std::string stop_condition;
    std::size_t event_limit{};
    std::vector<ExternalTraceEvent> events;
};

struct ResetVectorEvidence {
    std::uint32_t initial_sp{};
    std::uint32_t initial_pc{};
};

struct EmulatorTraceMetadata {
    std::string rom_id;
    std::string rom_sha256;
    std::string emulator;
    std::string backend;
    std::string version;
    std::string scenario;
    std::string stop_condition;
    std::size_t event_limit{};
};

struct TraceCoverageRange {
    std::uint32_t start{};
    std::uint32_t end{};
    std::string evidence;
};

struct DirectCallEdge {
    std::uint32_t caller{};
    std::uint32_t callee{};
    std::size_t count{};
};

struct EmulatorTraceReport {
    EmulatorTraceMetadata metadata;
    std::vector<ExternalTraceEvent> events;
    std::vector<std::uint32_t> unique_pcs;
    std::vector<std::uint32_t> executed_basic_blocks;
    std::vector<TraceCoverageRange> executed_ranges;
    std::vector<DirectCallEdge> direct_call_edges;
    std::vector<std::uint32_t> indirect_targets;
    std::vector<std::uint32_t> atlas_known_pcs;
    std::vector<std::uint32_t> atlas_unknown_pcs;
    std::vector<std::uint32_t> atlas_known_control_flow_targets;
    std::vector<std::uint32_t> atlas_unknown_control_flow_targets;
    std::optional<ResetVectorEvidence> reset_vectors;
    std::optional<std::uint32_t> first_observed_pc;
    std::optional<bool> first_pc_matches_reset;
    std::size_t branch_count{};
    std::size_t call_count{};
    std::size_t return_count{};
    std::size_t memory_read_count{};
    std::size_t memory_write_count{};
    std::vector<std::string> deterministic_fields;
    std::vector<std::string> nondeterministic_fields;
    std::string trace_hash;
};

[[nodiscard]] ExternalTraceCapture parse_external_trace(std::string_view text);
[[nodiscard]] ResetVectorEvidence read_reset_vectors(std::span<const std::uint8_t> rom);
[[nodiscard]] EmulatorTraceReport normalize_emulator_trace(
    ExternalTraceCapture capture, std::span<const std::uint32_t> atlas_code_addresses,
    std::optional<ResetVectorEvidence> reset_vectors = std::nullopt,
    std::string rom_id = {}, std::string rom_sha256 = {});

[[nodiscard]] std::string emulator_trace_to_json(const EmulatorTraceReport& report);
[[nodiscard]] std::string emulator_trace_to_text(const EmulatorTraceReport& report);

} // namespace oasis::tools
