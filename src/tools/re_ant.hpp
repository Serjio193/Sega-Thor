#pragma once

#include "tools/re_emulator_trace.hpp"
#include "tools/re_explore.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace oasis::tools {

enum class AntStatus { resolved, not_reached, timeout, backend_limitation, diverged, error };
enum class AntReachabilityClass { natural_observed, checkpoint_replay_observed, state_guided_observed, forced };

struct AntJob {
    std::string schema{"oasis.m68k.re-ant-job.v1"};
    std::string job_id;
    std::string frontier_id;
    std::string rom_sha256;
    std::size_t rom_size{};
    std::string backend{"bizhawk"};
    std::string backend_version;
    std::string scenario_id;
    std::string start_state{"hardware_reset"};
    std::string input_events;
    std::string checkpoint_reference{"none"};
    std::uint32_t source_entry{};
    std::uint32_t source_pc{};
    std::string blocker_type;
    std::vector<std::uint8_t> instruction_bytes;
    std::string instruction;
    std::string known_static_context;
    std::string expected_observation_type{"RESOLVE_INDIRECT_TARGET"};
    std::size_t max_steps{};
    std::size_t max_frames{};
    std::string allowed_input_policy{"neutral_only"};
    std::string provenance_requirement{"NATURAL_OBSERVED"};
};

struct AntObservation {
    std::uint32_t actual_pc{};
    std::string instruction;
    std::vector<std::uint8_t> instruction_bytes;
    EmulatorRegisterSnapshot registers{};
    std::optional<std::uint32_t> next_pc;
    std::optional<std::uint32_t> indirect_target;
    std::uint64_t frame{};
    std::uint64_t sequence{};
    bool target_inside_rom{};
};

struct AntResult {
    std::string schema{"oasis.m68k.re-ant-result.v1"};
    std::string job_id;
    std::string frontier_id;
    AntStatus status{AntStatus::error};
    std::string backend;
    std::string backend_version;
    std::string rom_sha256;
    AntReachabilityClass reachability_class{AntReachabilityClass::forced};
    std::uint32_t source_entry{};
    std::uint32_t source_pc{};
    std::optional<AntObservation> observed;
    std::string stop_reason;
    bool reproducible{};
    std::string result_hash;
    std::size_t startup_load_ms{};
    std::size_t checkpoint_restore_ms{};
    std::size_t execution_ms{};
    std::size_t total_wall_clock_ms{};
    std::size_t frames_executed{};
    std::size_t instructions_until_observation{};
};

struct AntMerge {
    bool accepted{};
    std::string reason;
    DynamicEdgeEvidence edge;
};

[[nodiscard]] AntJob make_ant_job(const FrontierRecord& frontier, std::string rom_sha256,
                                  std::size_t rom_size, std::string backend_version);
[[nodiscard]] std::optional<FrontierRecord> select_ant_frontier(
    std::string_view explore_json, std::uint32_t source_pc);
[[nodiscard]] AntJob parse_ant_job(std::string_view json);
[[nodiscard]] AntResult parse_ant_result(std::string_view json);
[[nodiscard]] AntMerge merge_ant_result(const AntJob& job, const AntResult& result,
                                        std::size_t rom_size, std::string_view rom_sha256);
[[nodiscard]] bool ant_results_equal(const AntResult& left, const AntResult& right);
[[nodiscard]] std::string ant_status_name(AntStatus value);
[[nodiscard]] std::string ant_reachability_name(AntReachabilityClass value);
[[nodiscard]] std::string ant_job_to_json(const AntJob& job);
[[nodiscard]] std::string ant_result_to_json(const AntResult& result);
[[nodiscard]] std::string ant_result_to_text(const AntResult& result);

} // namespace oasis::tools
