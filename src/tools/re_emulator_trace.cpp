#include "tools/re_emulator_trace.hpp"

#include <algorithm>
#include <charconv>
#include <map>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace oasis::tools {
namespace {

std::uint64_t number(std::string_view text, int base) {
    if (text.empty()) throw std::invalid_argument("empty trace number");
    if (base == 16 && text.size() > 2U && text.substr(0, 2) == "0x") text.remove_prefix(2);
    std::uint64_t value{};
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value, base);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size())
        throw std::invalid_argument("invalid trace number: " + std::string(text));
    return value;
}

std::string required(const std::map<std::string, std::string>& fields, const char* key) {
    const auto found = fields.find(key);
    if (found == fields.end() || found->second.empty()) throw std::invalid_argument(std::string("missing trace field: ") + key);
    return found->second;
}

EmulatorEventKind event_kind(std::string_view value) {
    if (value == "instruction") return EmulatorEventKind::instruction;
    if (value == "branch") return EmulatorEventKind::branch;
    if (value == "call") return EmulatorEventKind::call;
    if (value == "return") return EmulatorEventKind::return_instruction;
    if (value == "read") return EmulatorEventKind::memory_read;
    if (value == "write") return EmulatorEventKind::memory_write;
    if (value == "indirect") return EmulatorEventKind::indirect_control_flow;
    throw std::invalid_argument("unknown external trace event kind: " + std::string(value));
}

std::string event_kind_name(EmulatorEventKind kind) {
    switch (kind) {
    case EmulatorEventKind::instruction: return "instruction";
    case EmulatorEventKind::branch: return "branch";
    case EmulatorEventKind::call: return "call";
    case EmulatorEventKind::return_instruction: return "return";
    case EmulatorEventKind::memory_read: return "read";
    case EmulatorEventKind::memory_write: return "write";
    case EmulatorEventKind::indirect_control_flow: return "indirect";
    }
    return "unknown";
}

std::optional<bool> boolean_field(const std::map<std::string, std::string>& fields, const char* key) {
    const auto found = fields.find(key);
    if (found == fields.end()) return std::nullopt;
    if (found->second == "true" || found->second == "1") return true;
    if (found->second == "false" || found->second == "0") return false;
    throw std::invalid_argument(std::string("invalid boolean trace field: ") + key);
}

ExternalTraceEvent parse_event(const std::vector<std::string>& tokens) {
    std::map<std::string, std::string> fields;
    for (std::size_t index = 1; index < tokens.size(); ++index) {
        const auto separator = tokens[index].find('=');
        if (separator == std::string::npos || separator == 0U)
            throw std::invalid_argument("malformed external trace event field");
        const auto key = tokens[index].substr(0, separator);
        if (!fields.emplace(key, tokens[index].substr(separator + 1U)).second)
            throw std::invalid_argument("duplicate external trace event field: " + key);
    }
    ExternalTraceEvent event{.sequence = number(required(fields, "seq"), 10),
                             .pc = static_cast<std::uint32_t>(number(required(fields, "pc"), 16)),
                             .kind = event_kind(required(fields, "kind"))};
    if (const auto found = fields.find("frame"); found != fields.end()) event.frame = number(found->second, 10);
    if (const auto found = fields.find("cycles"); found != fields.end()) event.cycles = number(found->second, 10);
    if (const auto found = fields.find("block"); found != fields.end()) event.block_start = static_cast<std::uint32_t>(number(found->second, 16));
    if (const auto found = fields.find("target"); found != fields.end()) event.target = static_cast<std::uint32_t>(number(found->second, 16));
    event.taken = boolean_field(fields, "taken");
    if (const auto found = fields.find("address"); found != fields.end()) event.address = static_cast<std::uint32_t>(number(found->second, 16));
    if (const auto found = fields.find("width"); found != fields.end()) event.width_bytes = static_cast<std::uint8_t>(number(found->second, 10));
    if (const auto found = fields.find("size"); found != fields.end()) event.instruction_size = static_cast<std::uint8_t>(number(found->second, 10));
    EmulatorRegisterSnapshot snapshot{};
    bool has_snapshot = false;
    for (std::size_t index = 0; index < 8U; ++index) {
        const auto key = "d" + std::to_string(index);
        if (const auto found = fields.find(key); found != fields.end()) {
            snapshot.d[index] = static_cast<std::uint32_t>(number(found->second, 16));
            has_snapshot = true;
        }
    }
    for (std::size_t index = 0; index < 8U; ++index) {
        const auto key = "a" + std::to_string(index);
        if (const auto found = fields.find(key); found != fields.end()) {
            snapshot.a[index] = static_cast<std::uint32_t>(number(found->second, 16));
            has_snapshot = true;
        }
    }
    if (const auto found = fields.find("sr"); found != fields.end()) {
        snapshot.sr = static_cast<std::uint16_t>(number(found->second, 16));
        has_snapshot = true;
    }
    if (has_snapshot) event.registers = snapshot;
    if (event.kind == EmulatorEventKind::branch && !event.target)
        throw std::invalid_argument("branch event requires target");
    if ((event.kind == EmulatorEventKind::call || event.kind == EmulatorEventKind::indirect_control_flow) && !event.target)
        throw std::invalid_argument("control-flow event requires target");
    if ((event.kind == EmulatorEventKind::memory_read || event.kind == EmulatorEventKind::memory_write) && (!event.address || event.width_bytes == 0U))
        throw std::invalid_argument("memory event requires address and nonzero width");
    return event;
}

std::vector<std::string> tokens(std::string_view line) {
    std::istringstream input{std::string(line)};
    std::vector<std::string> result;
    for (std::string token; input >> token;) result.push_back(std::move(token));
    return result;
}

void unique_sorted(std::vector<std::uint32_t>& values) {
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
}

std::string kind_name(EmulatorEventKind kind) { return event_kind_name(kind); }

std::uint64_t fnv_append(std::uint64_t hash, std::string_view value) {
    for (const auto character : value) {
        hash ^= static_cast<std::uint8_t>(character);
        hash *= 1099511628211ULL;
    }
    return hash;
}

void hash_optional(std::uint64_t& hash, const std::optional<std::uint32_t>& value) {
    hash = fnv_append(hash, value ? std::to_string(*value) : "unknown");
    hash = fnv_append(hash, ";");
}

std::string hex64(std::uint64_t value) {
    std::ostringstream output;
    output << "0x" << std::hex << std::uppercase << value;
    return output.str();
}

} // namespace

ExternalTraceCapture parse_external_trace(std::string_view text) {
    ExternalTraceCapture result;
    bool header = false;
    std::istringstream input{std::string(text)};
    for (std::string line; std::getline(input, line);) {
        const auto first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos || line[first] == '#') continue;
        auto content = std::string_view(line).substr(first);
        if (!content.empty() && content.back() == '\r') content.remove_suffix(1);
        if (content == "oasis.m68k.external-trace.v1") { header = true; continue; }
        const auto fields = tokens(content);
        if (fields.empty()) continue;
        if (fields.front() == "event") {
            if (!header) throw std::invalid_argument("external trace header must precede events");
            result.events.push_back(parse_event(fields));
            continue;
        }
        if (fields.size() != 1U) throw std::invalid_argument("malformed external trace metadata");
        const auto separator = fields.front().find('=');
        if (separator == std::string::npos) throw std::invalid_argument("malformed external trace metadata");
        const auto key = fields.front().substr(0, separator);
        const auto value = fields.front().substr(separator + 1U);
        if (key == "emulator") result.emulator = value;
        else if (key == "backend") result.backend = value;
        else if (key == "version") result.version = value;
        else if (key == "scenario") result.scenario = value;
        else if (key == "stop_condition") result.stop_condition = value;
        else if (key == "limit") result.event_limit = static_cast<std::size_t>(number(value, 10));
        else throw std::invalid_argument("unknown external trace metadata: " + key);
    }
    if (!header) throw std::invalid_argument("missing external trace header");
    if (result.events.empty()) throw std::invalid_argument("external trace has no events");
    return result;
}

ResetVectorEvidence read_reset_vectors(std::span<const std::uint8_t> rom) {
    if (rom.size() < 8U) throw std::invalid_argument("ROM is too small for reset vectors");
    const auto read32 = [&](std::size_t offset) {
        return (static_cast<std::uint32_t>(rom[offset]) << 24U) |
            (static_cast<std::uint32_t>(rom[offset + 1U]) << 16U) |
            (static_cast<std::uint32_t>(rom[offset + 2U]) << 8U) | rom[offset + 3U];
    };
    return {read32(0), read32(4)};
}

EmulatorTraceReport normalize_emulator_trace(
    ExternalTraceCapture capture, std::span<const std::uint32_t> atlas_code_addresses,
    std::optional<ResetVectorEvidence> reset_vectors, std::string rom_id, std::string rom_sha256) {
    std::sort(capture.events.begin(), capture.events.end(), [](const auto& left, const auto& right) {
        return std::tie(left.sequence, left.pc, left.target) < std::tie(right.sequence, right.pc, right.target);
    });
    for (std::size_t index = 1; index < capture.events.size(); ++index)
        if (capture.events[index - 1U].sequence == capture.events[index].sequence)
            throw std::invalid_argument("duplicate external trace sequence");

    EmulatorTraceReport report{.metadata = {std::move(rom_id), std::move(rom_sha256), std::move(capture.emulator),
                                             std::move(capture.backend), std::move(capture.version),
                                             std::move(capture.scenario), std::move(capture.stop_condition), capture.event_limit},
                               .events = std::move(capture.events), .reset_vectors = reset_vectors};
    report.deterministic_fields = {"sequence", "pc", "kind", "block", "target", "taken", "address", "width_bytes", "instruction_size", "registers"};
    report.nondeterministic_fields = {"frame", "cycles"};
    for (const auto& event : report.events) {
        report.unique_pcs.push_back(event.pc);
        if (event.block_start) report.executed_basic_blocks.push_back(*event.block_start);
        switch (event.kind) {
        case EmulatorEventKind::branch: ++report.branch_count; break;
        case EmulatorEventKind::call: ++report.call_count; break;
        case EmulatorEventKind::return_instruction: ++report.return_count; break;
        case EmulatorEventKind::memory_read: ++report.memory_read_count; break;
        case EmulatorEventKind::memory_write: ++report.memory_write_count; break;
        case EmulatorEventKind::indirect_control_flow:
            if (event.target) report.indirect_targets.push_back(*event.target);
            break;
        case EmulatorEventKind::instruction: break;
        }
        if (event.kind == EmulatorEventKind::call && event.target) {
            const auto found = std::find_if(report.direct_call_edges.begin(), report.direct_call_edges.end(), [&](const auto& edge) {
                return edge.caller == event.pc && edge.callee == *event.target;
            });
            if (found == report.direct_call_edges.end()) report.direct_call_edges.push_back({event.pc, *event.target, 1});
            else ++found->count;
        }
        if ((event.kind == EmulatorEventKind::branch || event.kind == EmulatorEventKind::call ||
             event.kind == EmulatorEventKind::indirect_control_flow) && event.target) {
            if (std::find(atlas_code_addresses.begin(), atlas_code_addresses.end(), *event.target) != atlas_code_addresses.end())
                report.atlas_known_control_flow_targets.push_back(*event.target);
            else report.atlas_unknown_control_flow_targets.push_back(*event.target);
        }
    }
    unique_sorted(report.unique_pcs);
    unique_sorted(report.executed_basic_blocks);
    unique_sorted(report.indirect_targets);
    unique_sorted(report.atlas_known_control_flow_targets);
    unique_sorted(report.atlas_unknown_control_flow_targets);
    std::vector<std::pair<std::uint32_t, std::uint8_t>> sized_pcs;
    for (const auto pc : report.unique_pcs) {
        const auto found = std::find_if(report.events.begin(), report.events.end(), [=](const auto& event) { return event.pc == pc && event.instruction_size != 0U; });
        if (found == report.events.end()) report.executed_ranges.push_back({pc, pc, "pc_observed_only"});
        else sized_pcs.push_back({pc, found->instruction_size});
    }
    for (const auto [pc, size] : sized_pcs) report.executed_ranges.push_back({pc, pc + size, "instruction_size_observed"});
    std::sort(report.direct_call_edges.begin(), report.direct_call_edges.end(), [](const auto& left, const auto& right) { return std::tie(left.caller, left.callee) < std::tie(right.caller, right.callee); });
    for (const auto pc : report.unique_pcs) {
        if (std::find(atlas_code_addresses.begin(), atlas_code_addresses.end(), pc) != atlas_code_addresses.end()) report.atlas_known_pcs.push_back(pc);
        else report.atlas_unknown_pcs.push_back(pc);
    }
    if (!report.events.empty()) report.first_observed_pc = report.events.front().pc;
    if (report.reset_vectors && report.first_observed_pc) report.first_pc_matches_reset = *report.first_observed_pc == report.reset_vectors->initial_pc;
    std::uint64_t hash = 1469598103934665603ULL;
    for (const auto& event : report.events) {
        hash = fnv_append(hash, std::to_string(event.sequence) + ":" + std::to_string(event.pc) + ":" + kind_name(event.kind));
        if (event.target) hash = fnv_append(hash, ":t=" + std::to_string(*event.target));
        if (event.taken) hash = fnv_append(hash, *event.taken ? ":taken=1" : ":taken=0");
        if (event.address) hash = fnv_append(hash, ":a=" + std::to_string(*event.address));
        hash = fnv_append(hash, ":w=" + std::to_string(event.width_bytes) + ":s=" + std::to_string(event.instruction_size) + ";");
        if (!event.registers) {
            hash = fnv_append(hash, ":regs=null;");
        } else {
            hash = fnv_append(hash, ":d=");
            for (const auto& value : event.registers->d) hash_optional(hash, value);
            hash = fnv_append(hash, ":a=");
            for (const auto& value : event.registers->a) hash_optional(hash, value);
            hash = fnv_append(hash, ":sr=");
            if (event.registers->sr) hash = fnv_append(hash, std::to_string(*event.registers->sr));
            else hash = fnv_append(hash, "unknown");
            hash = fnv_append(hash, ";");
        }
    }
    report.trace_hash = hex64(hash);
    return report;
}

} // namespace oasis::tools
