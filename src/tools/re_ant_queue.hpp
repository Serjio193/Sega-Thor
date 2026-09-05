#pragma once

#include "tools/re_ant.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace oasis::tools {

enum class AntQueueLifecycle { available, claimed, resolved, failed_retryable, failed_final };

struct AntReachabilityHint {
    std::uint32_t source_pc{};
    std::size_t first_frame{};
    std::size_t hit_count{};
};

struct AntQueueJob {
    AntJob job;
    AntQueueLifecycle lifecycle{AntQueueLifecycle::available};
    std::size_t attempts{};
    std::string last_result_hash;
    std::string failure_reason;
};

struct AntQueue {
    std::string schema{"oasis.m68k.re-ant-queue.v1"};
    std::string queue_id;
    std::string rom_sha256;
    std::size_t rom_size{};
    std::string backend{"bizhawk"};
    std::string backend_version;
    std::string created_from_explorer_hash;
    std::string generation_policy{"natural_reachability_then_source_address_v1"};
    std::string queue_state{"FROZEN"};
    std::size_t version{1};
    std::size_t duplicate_jobs_avoided{};
    std::vector<AntQueueJob> jobs;
};

[[nodiscard]] std::vector<FrontierRecord> select_ant_frontiers(std::string_view explore_json);
[[nodiscard]] std::vector<AntReachabilityHint> parse_ant_reachability(std::string_view json);
[[nodiscard]] AntQueue make_ant_queue(std::string_view explore_json,
                                      std::string_view reachability_json,
                                      std::string rom_sha256, std::size_t rom_size,
                                      std::string backend_version,
                                      std::size_t max_jobs = 5);
[[nodiscard]] AntQueue parse_ant_queue(std::string_view json);
[[nodiscard]] bool add_ant_queue_job(AntQueue& queue, AntJob job);
[[nodiscard]] std::optional<std::size_t> claim_next_ant_job(AntQueue& queue);
void recover_stale_ant_claims(AntQueue& queue);
void finalize_ant_job(AntQueue& queue, std::size_t index, const AntResult& result,
                      bool accepted, std::string reason);
[[nodiscard]] bool ant_queue_equal(const AntQueue& left, const AntQueue& right);
[[nodiscard]] std::string ant_queue_lifecycle_name(AntQueueLifecycle value);
[[nodiscard]] std::string ant_queue_to_json(const AntQueue& queue);
[[nodiscard]] std::string ant_queue_to_text(const AntQueue& queue);

} // namespace oasis::tools
