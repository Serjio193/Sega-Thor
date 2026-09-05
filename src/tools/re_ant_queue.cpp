#include "tools/re_ant_queue.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace oasis::tools {
namespace {

std::string field(std::string_view json, std::string_view key) {
    const auto marker = "\"" + std::string(key) + "\":";
    const auto start = json.find(marker);
    if (start == std::string_view::npos) throw std::invalid_argument("missing queue field: " + std::string(key));
    auto value = json.substr(start + marker.size());
    if (value.starts_with("\"")) {
        value.remove_prefix(1);
        const auto end = value.find('"');
        if (end == std::string_view::npos) throw std::invalid_argument("unterminated queue string");
        return std::string(value.substr(0, end));
    }
    const auto end = value.find_first_of(",}");
    return std::string(value.substr(0, end));
}

std::size_t decimal(std::string_view value) {
    std::size_t result{};
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result, 10);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size()) throw std::invalid_argument("invalid queue number");
    return result;
}

std::uint32_t hex_number(std::string_view value) {
    if (value.starts_with("0x") || value.starts_with("0X")) value.remove_prefix(2);
    std::uint32_t result{};
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result, 16);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size()) throw std::invalid_argument("invalid queue address");
    return result;
}

std::string object_at(std::string_view json, std::size_t start) {
    bool quoted = false;
    unsigned depth = 0;
    for (std::size_t index = start; index < json.size(); ++index) {
        const char current = json[index];
        if (current == '"' && (index == 0U || json[index - 1U] != '\\')) quoted = !quoted;
        if (quoted) continue;
        if (current == '{') ++depth;
        if (current == '}' && --depth == 0U) return std::string(json.substr(start, index - start + 1U));
    }
    throw std::invalid_argument("unterminated queue object");
}

std::string hex64(std::uint64_t value) {
    std::ostringstream out;
    out << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(16) << value;
    return out.str();
}

std::uint64_t hash_text(std::string_view text) {
    std::uint64_t hash = 1469598103934665603ULL;
    for (const auto byte : text) { hash ^= static_cast<std::uint8_t>(byte); hash *= 1099511628211ULL; }
    return hash;
}

AntQueueLifecycle job_lifecycle_from_name(std::string_view value);

std::optional<AntReachabilityHint> reachability_for(std::string_view json, std::uint32_t source_pc) {
    std::ostringstream marker;
    marker << '"' << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << source_pc << "\":{";
    const auto start = json.find(marker.str());
    if (start == std::string_view::npos) return std::nullopt;
    const auto object = object_at(json, start + marker.str().size() - 1U);
    const auto count = decimal(field(object, "count"));
    if (!count) return std::nullopt;
    return AntReachabilityHint{source_pc, decimal(field(object, "frame")), count};
}

bool has_dynamic_edge(std::string_view json, const FrontierRecord& frontier) {
    std::ostringstream marker;
    marker << "\"source_pc\":\"0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << frontier.source_pc << "\"";
    auto position = json.find("\"kind\":\"DYNAMIC_INDIRECT\"");
    while (position != std::string_view::npos) {
        const auto start = json.rfind('{', position);
        const auto end = json.find('}', position);
        if (start != std::string_view::npos && end != std::string_view::npos &&
            json.substr(start, end - start + 1U).find(marker.str()) != std::string_view::npos)
            return true;
        position = json.find("\"kind\":\"DYNAMIC_INDIRECT\"", position + 1U);
    }
    return false;
}

AntQueueLifecycle job_lifecycle_from_name(std::string_view value) {
    if (value == "AVAILABLE") return AntQueueLifecycle::available;
    if (value == "CLAIMED") return AntQueueLifecycle::claimed;
    if (value == "RESOLVED") return AntQueueLifecycle::resolved;
    if (value == "FAILED_RETRYABLE") return AntQueueLifecycle::failed_retryable;
    if (value == "FAILED_FINAL") return AntQueueLifecycle::failed_final;
    throw std::invalid_argument("invalid queue lifecycle");
}

} // namespace

std::vector<FrontierRecord> select_ant_frontiers(std::string_view explore_json) {
    const auto start = explore_json.find("\"frontier\":");
    if (start == std::string_view::npos) return {};
    const auto frontier_end = explore_json.find("],\"address_map\"", start);
    std::vector<FrontierRecord> result;
    auto position = start;
    while ((position = explore_json.find("\"source_pc\":\"0x", position)) != std::string_view::npos &&
           (frontier_end == std::string_view::npos || position < frontier_end)) {
        const auto address_start = position + std::string_view("\"source_pc\":\"0x").size();
        const auto address_end = explore_json.find('"', address_start);
        if (address_end == std::string_view::npos) break;
        const auto source_pc = hex_number(explore_json.substr(address_start, address_end - address_start));
        const auto object_start = explore_json.rfind('{', position);
        const auto object = object_start == std::string_view::npos ? std::string{} : object_at(explore_json, object_start);
        if (const auto frontier = select_ant_frontier(object, source_pc);
            frontier && std::none_of(result.begin(), result.end(), [&](const auto& item) { return item.id == frontier->id; }))
            result.push_back(*frontier);
        position = address_end + 1U;
    }
    return result;
}

std::vector<AntReachabilityHint> parse_ant_reachability(std::string_view json) {
    const auto start = json.find("\"hits\":{");
    if (start == std::string_view::npos) return {};
    const auto hits_end = json.find("}}", start);
    std::vector<AntReachabilityHint> result;
    auto position = start + 8U;
    while ((position = json.find("\"0x", position)) != std::string_view::npos &&
           (hits_end == std::string_view::npos || position < hits_end)) {
        const auto address_start = position + 3U;
        const auto address_end = json.find('"', address_start);
        if (address_end == std::string_view::npos) break;
        const auto address = hex_number(json.substr(address_start, address_end - address_start));
        const auto hint = reachability_for(json, address);
        if (hint) result.push_back(*hint);
        position = address_end + 1U;
    }
    return result;
}

AntQueue make_ant_queue(std::string_view explore_json, std::string_view reachability_json,
                        std::string rom_sha256, std::size_t rom_size,
                        std::string backend_version, std::size_t max_jobs) {
    if (max_jobs < 5U || max_jobs > 10U) throw std::invalid_argument("queue must contain 5 to 10 jobs");
    const auto frontiers = select_ant_frontiers(explore_json);
    const auto hints = parse_ant_reachability(reachability_json);
    const auto hint_for = [&](const FrontierRecord& item) -> std::optional<AntReachabilityHint> {
        for (const auto& hint : hints) if (hint.source_pc == item.source_pc) return hint;
        return std::nullopt;
    };
    std::vector<FrontierRecord> candidates;
    for (const auto& item : frontiers) {
        if (item.blocker_type == FrontierType::indirect_flow && !has_dynamic_edge(explore_json, item)) candidates.push_back(item);
    }
    std::stable_sort(candidates.begin(), candidates.end(), [&](const auto& left, const auto& right) {
        const auto l = hint_for(left), r = hint_for(right);
        if (l.has_value() != r.has_value()) return l.has_value() > r.has_value();
        if (l && r && l->first_frame != r->first_frame) return l->first_frame < r->first_frame;
        if (left.source_pc != right.source_pc) return left.source_pc < right.source_pc;
        return left.source_entry < right.source_entry;
    });
    AntQueue queue;
    queue.queue_state = "BUILDING";
    queue.rom_sha256 = std::move(rom_sha256);
    queue.rom_size = rom_size;
    queue.backend_version = std::move(backend_version);
    std::string identity_input = std::string(explore_json) + "|" + std::string(reachability_json) + "|" + queue.rom_sha256 + "|" + queue.generation_policy;
    for (std::size_t index = 0; index < std::min(max_jobs, candidates.size()); ++index) {
        const auto hint = hint_for(candidates[index]);
        const auto job = hint ? make_ant_job_for_scenario(candidates[index], queue.rom_sha256, queue.rom_size,
            queue.backend_version, "natural_idle_to_6121a_v1", field(reachability_json, "input_events"),
            decimal(field(reachability_json, "frames")), 8000000) : make_ant_job_for_scenario(candidates[index], queue.rom_sha256,
            queue.rom_size, queue.backend_version, "natural_idle_to_6121a_v1", {}, 300, 3000000);
        identity_input += '|' + job.job_id;
        if (!add_ant_queue_job(queue, job)) throw std::logic_error("queue selection produced a duplicate job");
    }
    if (queue.jobs.size() != max_jobs) throw std::invalid_argument("not enough INDIRECT_FLOW frontiers for queue: " + std::to_string(candidates.size()));
    queue.queue_state = "FROZEN";
    queue.created_from_explorer_hash = "fnv1a64:" + hex64(hash_text(explore_json));
    queue.queue_id = "queue-" + hex64(hash_text(identity_input));
    return queue;
}

AntQueue parse_ant_queue(std::string_view json) {
    AntQueue result{.schema = field(json, "schema"), .queue_id = field(json, "queue_id"),
                    .rom_sha256 = field(json, "rom_sha256"), .rom_size = decimal(field(json, "rom_size")),
                    .backend = field(json, "backend"), .backend_version = field(json, "backend_version"),
                    .created_from_explorer_hash = field(json, "created_from_explorer_hash"),
                    .generation_policy = field(json, "generation_policy"), .queue_state = field(json, "queue_state"),
                    .version = decimal(field(json, "version")), .duplicate_jobs_avoided = decimal(field(json, "duplicate_jobs_avoided"))};
    if (result.schema != "oasis.m68k.re-ant-queue.v1") throw std::invalid_argument("wrong ant queue schema");
    const auto jobs_start = json.find("\"jobs\":[");
    if (jobs_start == std::string_view::npos) throw std::invalid_argument("missing queue jobs");
    auto position = jobs_start;
    while ((position = json.find("\"job\":{", position)) != std::string_view::npos) {
        const auto wrapper_start = json.rfind('{', position);
        const auto wrapper = object_at(json, wrapper_start);
        const auto job_start = position + 6U;
        const auto job = parse_ant_job(object_at(json, job_start));
        result.jobs.push_back({job, job_lifecycle_from_name(field(wrapper, "state")), decimal(field(wrapper, "attempts")),
                               field(wrapper, "last_result_hash"), field(wrapper, "failure_reason")});
        position = wrapper_start + wrapper.size();
    }
    return result;
}

bool add_ant_queue_job(AntQueue& queue, AntJob job) {
    if (queue.queue_state != "BUILDING") throw std::logic_error("frozen queue cannot absorb jobs");
    const auto duplicate = std::find_if(queue.jobs.begin(), queue.jobs.end(), [&](const auto& item) {
        return item.job.frontier_id == job.frontier_id && item.job.rom_sha256 == job.rom_sha256 &&
            item.job.scenario_id == job.scenario_id && item.job.input_events == job.input_events;
    });
    if (duplicate != queue.jobs.end()) {
        if (duplicate->lifecycle == AntQueueLifecycle::resolved) ++queue.duplicate_jobs_avoided;
        return false;
    }
    queue.jobs.push_back({std::move(job)});
    return true;
}

std::optional<std::size_t> claim_next_ant_job(AntQueue& queue) {
    if (std::any_of(queue.jobs.begin(), queue.jobs.end(), [](const auto& item) { return item.lifecycle == AntQueueLifecycle::claimed; }))
        throw std::logic_error("queue already has a CLAIMED job");
    for (std::size_t index = 0; index < queue.jobs.size(); ++index) {
        auto& item = queue.jobs[index];
        if (item.lifecycle == AntQueueLifecycle::available) { item.lifecycle = AntQueueLifecycle::claimed; ++item.attempts; return index; }
    }
    return std::nullopt;
}

void recover_stale_ant_claims(AntQueue& queue) {
    for (auto& item : queue.jobs) if (item.lifecycle == AntQueueLifecycle::claimed) {
        item.lifecycle = AntQueueLifecycle::failed_retryable;
        item.failure_reason = "stale_claim_recovered";
    }
}

void finalize_ant_job(AntQueue& queue, std::size_t index, const AntResult& result,
                      bool accepted, std::string reason) {
    if (index >= queue.jobs.size() || queue.jobs[index].lifecycle != AntQueueLifecycle::claimed)
        throw std::logic_error("finalize requires a CLAIMED job");
    auto& item = queue.jobs[index];
    item.lifecycle = accepted ? AntQueueLifecycle::resolved :
        (reason.starts_with("retryable:") && item.attempts <= 1U ? AntQueueLifecycle::failed_retryable : AntQueueLifecycle::failed_final);
    item.last_result_hash = result.result_hash;
    item.failure_reason = std::move(reason);
}

bool ant_queue_equal(const AntQueue& left, const AntQueue& right) { return ant_queue_to_json(left) == ant_queue_to_json(right); }

std::string ant_queue_lifecycle_name(AntQueueLifecycle value) {
    switch (value) { case AntQueueLifecycle::available: return "AVAILABLE"; case AntQueueLifecycle::claimed: return "CLAIMED"; case AntQueueLifecycle::resolved: return "RESOLVED"; case AntQueueLifecycle::failed_retryable: return "FAILED_RETRYABLE"; case AntQueueLifecycle::failed_final: return "FAILED_FINAL"; }
    return "FAILED_FINAL";
}

} // namespace oasis::tools
