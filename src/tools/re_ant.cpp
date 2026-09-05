#include "tools/re_ant.hpp"

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

std::uint64_t hash_append(std::uint64_t hash, std::string_view value) {
    for (const auto byte : value) { hash ^= static_cast<std::uint8_t>(byte); hash *= 1099511628211ULL; }
    return hash;
}

std::string hash_hex(std::uint64_t value) {
    std::ostringstream out;
    out << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(16) << value;
    return out.str();
}

std::string hex32(std::uint32_t value) {
    std::ostringstream out;
    out << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << value;
    return out.str();
}

std::uint32_t number(std::string_view value) {
    std::uint32_t result{};
    if (value.starts_with("0x") || value.starts_with("0X")) value.remove_prefix(2);
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result, 16);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size()) throw std::invalid_argument("invalid ant number");
    return result;
}

std::size_t decimal(std::string_view value) {
    std::size_t result{};
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result, 10);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size()) throw std::invalid_argument("invalid ant decimal");
    return result;
}

std::string field(std::string_view json, std::string_view key) {
    const auto marker = "\"" + std::string(key) + "\":";
    const auto start = json.find(marker);
    if (start == std::string_view::npos) throw std::invalid_argument("missing ant field: " + std::string(key));
    auto value = json.substr(start + marker.size());
    if (value.starts_with("\"")) {
        value.remove_prefix(1);
        const auto end = value.find('"');
        if (end == std::string_view::npos) throw std::invalid_argument("unterminated ant string");
        return std::string(value.substr(0, end));
    }
    const auto end = value.find_first_of(",}");
    return std::string(value.substr(0, end));
}

std::string optional_field(std::string_view json, std::string_view key) {
    try { return field(json, key); } catch (const std::invalid_argument&) { return {}; }
}

bool boolean(std::string_view value) { return value == "true"; }

std::vector<std::uint8_t> byte_array(std::string_view json) {
    const std::string marker = "\"instruction_bytes\":[";
    const auto start = json.find(marker);
    if (start == std::string_view::npos) return {};
    auto value = json.substr(start + marker.size());
    const auto end = value.find(']');
    if (end == std::string_view::npos) throw std::invalid_argument("unterminated ant byte array");
    value = value.substr(0, end);
    std::vector<std::uint8_t> result;
    std::istringstream input{std::string(value)};
    for (unsigned item{}; input >> item;) {
        result.push_back(static_cast<std::uint8_t>(item));
        if (input.peek() == ',') input.ignore();
    }
    return result;
}

std::string source_object(std::string_view json, std::uint32_t source_pc) {
    const auto marker = "\"source_pc\":\"" + hex32(source_pc) + "\"";
    const auto position = json.find(marker);
    if (position == std::string_view::npos) return {};
    auto start = json.rfind('{', position);
    if (start == std::string_view::npos) return {};
    bool quoted = false;
    unsigned depth = 0;
    for (std::size_t index = start; index < json.size(); ++index) {
        const char current = json[index];
        if (current == '"' && (index == 0U || json[index - 1U] != '\\')) quoted = !quoted;
        if (quoted) continue;
        if (current == '{') ++depth;
        if (current == '}' && --depth == 0U) return std::string(json.substr(start, index - start + 1U));
    }
    return {};
}

std::uint64_t job_hash(const FrontierRecord& frontier, std::string_view rom_sha256,
                       std::string_view scenario_id, std::string_view input_events,
                       std::size_t max_frames, std::size_t max_steps) {
    std::uint64_t hash = 1469598103934665603ULL;
    const auto values = std::array<std::string, 10>{frontier.id, std::string(rom_sha256),
        std::to_string(frontier.source_entry), std::to_string(frontier.source_pc), frontier.known_context,
        "RESOLVE_INDIRECT_TARGET", std::string(scenario_id), std::string(input_events),
        std::to_string(max_frames), std::to_string(max_steps)};
    for (const auto& value : values) {
        hash = hash_append(hash, value); hash = hash_append(hash, "|");
    }
    return hash;
}

std::uint64_t normalized_hash(const AntResult& result) {
    std::uint64_t hash = 1469598103934665603ULL;
    const auto values = std::array<std::string, 8>{result.job_id, result.frontier_id,
        ant_status_name(result.status), ant_reachability_name(result.reachability_class),
        std::to_string(result.source_pc), result.observed && result.observed->indirect_target ?
        std::to_string(*result.observed->indirect_target) : "unknown",
        result.observed ? std::to_string(result.observed->frame) : "unknown",
        result.observed ? std::to_string(result.observed->sequence) : "unknown"};
    for (const auto& value : values) { hash = hash_append(hash, value); hash = hash_append(hash, "|"); }
    return hash;
}

} // namespace

AntJob make_ant_job(const FrontierRecord& frontier, std::string rom_sha256,
                   std::size_t rom_size, std::string backend_version) {
    return make_ant_job_for_scenario(frontier, std::move(rom_sha256), rom_size,
                                     std::move(backend_version),
                                     "natural_reset_idle_v1", {}, 300);
}

AntJob make_ant_job_for_scenario(const FrontierRecord& frontier, std::string rom_sha256,
                                std::size_t rom_size, std::string backend_version,
                                std::string scenario_id, std::string input_events,
                                std::size_t max_frames, std::size_t max_steps) {
    if (frontier.blocker_type != FrontierType::indirect_flow) throw std::invalid_argument("ant requires INDIRECT_FLOW frontier");
    AntJob result{.job_id = "ant-" + hash_hex(job_hash(frontier, rom_sha256, scenario_id, input_events, max_frames, max_steps)), .frontier_id = frontier.id,
                  .rom_sha256 = std::move(rom_sha256), .rom_size = rom_size, .backend_version = std::move(backend_version),
                  .scenario_id = std::move(scenario_id), .input_events = std::move(input_events),
                  .source_entry = frontier.source_entry,
                  .source_pc = frontier.source_pc, .blocker_type = frontier_type_name(frontier.blocker_type),
                  .instruction_bytes = frontier.instruction_bytes, .instruction = frontier.instruction,
                  .known_static_context = frontier.known_context, .max_steps = max_steps, .max_frames = max_frames};
    return result;
}

std::optional<FrontierRecord> select_ant_frontier(std::string_view explore_json, std::uint32_t source_pc) {
    const auto frontier_start = explore_json.find("\"frontier\":");
    const auto frontier_json = frontier_start == std::string_view::npos ? explore_json : explore_json.substr(frontier_start);
    const auto object = source_object(frontier_json, source_pc);
    if (object.empty() || optional_field(object, "blocker_type") != "INDIRECT_FLOW") return std::nullopt;
    FrontierRecord result{.id = field(object, "id"), .source_entry = number(field(object, "source_entry")),
                          .source_pc = number(field(object, "source_pc")), .blocker_type = FrontierType::indirect_flow,
                          .instruction_bytes = byte_array(object), .opcode = static_cast<std::uint16_t>(number(field(object, "opcode"))),
                          .instruction = field(object, "instruction"), .known_context = field(object, "known_context"),
                          .stop_reason = StopReason::indirect_transfer, .reason = field(object, "reason")};
    return result;
}

AntJob parse_ant_job(std::string_view json) {
    AntJob result{.schema = field(json, "schema"), .job_id = field(json, "job_id"), .frontier_id = field(json, "frontier_id"),
                  .rom_sha256 = field(json, "rom_sha256"), .rom_size = decimal(field(json, "rom_size")),
                  .backend = field(json, "backend"), .backend_version = field(json, "backend_version"),
                  .scenario_id = field(json, "scenario_id"), .start_state = field(json, "start_state"),
                  .input_events = field(json, "input_events"), .checkpoint_reference = field(json, "checkpoint_reference"),
                  .source_entry = number(field(json, "source_entry")), .source_pc = number(field(json, "source_pc")),
                  .blocker_type = field(json, "blocker_type"), .instruction_bytes = byte_array(json),
                  .instruction = field(json, "instruction"), .known_static_context = field(json, "known_static_context"),
                  .expected_observation_type = field(json, "expected_observation_type"),
                  .max_steps = decimal(field(json, "max_steps")), .max_frames = decimal(field(json, "max_frames")),
                  .allowed_input_policy = field(json, "allowed_input_policy"),
                  .provenance_requirement = field(json, "provenance_requirement")};
    if (result.schema != "oasis.m68k.re-ant-job.v1") throw std::invalid_argument("wrong ant job schema");
    return result;
}

AntResult parse_ant_result(std::string_view json) {
    AntResult result{.schema = field(json, "schema"), .job_id = field(json, "job_id"), .frontier_id = field(json, "frontier_id"),
                      .status = AntStatus::error, .backend = field(json, "backend"), .backend_version = field(json, "backend_version"),
                      .rom_sha256 = field(json, "rom_sha256"), .source_entry = number(field(json, "source_entry")),
                      .source_pc = number(field(json, "source_pc")), .stop_reason = field(json, "stop_reason"),
                      .reproducible = boolean(field(json, "reproducible")), .result_hash = field(json, "result_hash"),
                      .startup_load_ms = number(field(json, "startup_load_ms")), .checkpoint_restore_ms = number(field(json, "checkpoint_restore_ms")),
                      .execution_ms = decimal(field(json, "execution_ms")), .total_wall_clock_ms = decimal(field(json, "total_wall_clock_ms")),
                      .frames_executed = decimal(field(json, "frames_executed")), .instructions_until_observation = decimal(field(json, "instructions_until_observation"))};
    result.scenario_id = optional_field(json, "scenario_id");
    result.input_events = optional_field(json, "input_events");
    result.input_policy = optional_field(json, "input_policy");
    result.checkpoint_reference = optional_field(json, "checkpoint_reference");
    result.worker_run_id = optional_field(json, "worker_run_id");
    const auto status = field(json, "status");
    if (status == "RESOLVED") result.status = AntStatus::resolved;
    else if (status == "NOT_REACHED") result.status = AntStatus::not_reached;
    else if (status == "TIMEOUT") result.status = AntStatus::timeout;
    else if (status == "BACKEND_LIMITATION") result.status = AntStatus::backend_limitation;
    else if (status == "DIVERGED") result.status = AntStatus::diverged;
    const auto reachability = field(json, "reachability_class");
    if (reachability == "DYNAMIC_NATURAL") result.reachability_class = AntReachabilityClass::natural_observed;
    else if (reachability == "DYNAMIC_CHECKPOINT_REPLAY") result.reachability_class = AntReachabilityClass::checkpoint_replay_observed;
    else if (reachability == "DYNAMIC_STATE_GUIDED") result.reachability_class = AntReachabilityClass::state_guided_observed;
    if (optional_field(json, "observed_actual_pc").empty()) return result;
    AntObservation observed{.actual_pc = number(field(json, "observed_actual_pc")), .instruction = field(json, "observed_instruction"),
                            .instruction_bytes = byte_array(json), .next_pc = number(field(json, "observed_next_pc")),
                            .indirect_target = number(field(json, "observed_indirect_target")), .frame = decimal(field(json, "observed_frame")),
                            .sequence = decimal(field(json, "observed_sequence")), .target_inside_rom = boolean(field(json, "target_inside_rom"))};
    result.observed = observed;
    return result;
}

AntMerge merge_ant_result(const AntJob& job, const AntResult& result,
                         std::size_t rom_size, std::string_view rom_sha256) {
    AntMerge merge;
    if (job.schema != "oasis.m68k.re-ant-job.v1" || result.schema != "oasis.m68k.re-ant-result.v1") { merge.reason = "schema_mismatch"; return merge; }
    if (job.job_id != result.job_id || job.frontier_id != result.frontier_id) { merge.reason = "job_frontier_mismatch"; return merge; }
    if (job.rom_sha256 != rom_sha256 || result.rom_sha256 != rom_sha256 || job.rom_size != rom_size) { merge.reason = "rom_mismatch"; return merge; }
    if (result.reachability_class == AntReachabilityClass::forced || result.reachability_class == AntReachabilityClass::state_guided_observed) { merge.reason = "forced_or_state_guided_evidence"; return merge; }
    if (result.status != AntStatus::resolved || !result.observed || !result.observed->indirect_target) { merge.reason = "result_not_resolved"; return merge; }
    if (result.reproducible != true || result.backend != job.backend ||
        result.backend_version != job.backend_version || result.source_entry != job.source_entry ||
        (!result.scenario_id.empty() && result.scenario_id != job.scenario_id) ||
        (!result.input_events.empty() && result.input_events != job.input_events) ||
        (!result.input_policy.empty() && result.input_policy != job.allowed_input_policy) ||
        (!result.checkpoint_reference.empty() && result.checkpoint_reference != job.checkpoint_reference) ||
        result.observed->instruction != job.instruction ||
        result.observed->instruction_bytes != job.instruction_bytes) {
        merge.reason = "observation_context_mismatch"; return merge;
    }
    if (result.source_entry != job.source_entry || result.source_pc != job.source_pc || result.observed->actual_pc != job.source_pc) { merge.reason = "source_frontier_mismatch"; return merge; }
    if (!result.observed->target_inside_rom || *result.observed->indirect_target >= rom_size || (*result.observed->indirect_target & 1U)) { merge.reason = "target_not_rom"; return merge; }
    merge.accepted = true;
    merge.reason = "accepted_dynamic_natural_edge";
    merge.edge = {.frontier_id = job.frontier_id, .source_entry = job.source_entry, .source_pc = job.source_pc,
                  .target = *result.observed->indirect_target, .evidence_class = result.reachability_class == AntReachabilityClass::natural_observed ? "DYNAMIC_NATURAL" : "DYNAMIC_CHECKPOINT_REPLAY",
                  .job_id = job.job_id, .result_hash = result.result_hash, .backend = result.backend, .scenario = job.scenario_id};
    return merge;
}

bool ant_results_equal(const AntResult& left, const AntResult& right) {
    if (left.job_id != right.job_id || left.frontier_id != right.frontier_id || left.status != right.status ||
        left.rom_sha256 != right.rom_sha256 || left.source_pc != right.source_pc || left.result_hash != right.result_hash)
        return false;
    if (!left.observed || !right.observed) return left.observed.has_value() == right.observed.has_value();
    return left.observed->actual_pc == right.observed->actual_pc && left.observed->next_pc == right.observed->next_pc &&
        left.observed->indirect_target == right.observed->indirect_target && left.observed->frame == right.observed->frame &&
        left.observed->sequence == right.observed->sequence && left.observed->registers.a == right.observed->registers.a;
}

std::string ant_status_name(AntStatus value) {
    switch (value) { case AntStatus::resolved: return "RESOLVED"; case AntStatus::not_reached: return "NOT_REACHED"; case AntStatus::timeout: return "TIMEOUT"; case AntStatus::backend_limitation: return "BACKEND_LIMITATION"; case AntStatus::diverged: return "DIVERGED"; case AntStatus::error: return "ERROR"; }
    return "ERROR";
}

std::string ant_reachability_name(AntReachabilityClass value) {
    switch (value) { case AntReachabilityClass::natural_observed: return "DYNAMIC_NATURAL"; case AntReachabilityClass::checkpoint_replay_observed: return "DYNAMIC_CHECKPOINT_REPLAY"; case AntReachabilityClass::state_guided_observed: return "DYNAMIC_STATE_GUIDED"; case AntReachabilityClass::forced: return "FORCED_HYPOTHESIS"; }
    return "FORCED_HYPOTHESIS";
}

} // namespace oasis::tools
