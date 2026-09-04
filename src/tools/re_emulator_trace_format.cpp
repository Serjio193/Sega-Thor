#include "tools/re_emulator_trace.hpp"

#include <iomanip>
#include <sstream>

namespace oasis::tools {
namespace {

std::string hex32(std::uint32_t value) {
    std::ostringstream output;
    output << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << value;
    return output.str();
}

std::string json(const std::string& value) {
    std::string result = "\"";
    for (const auto character : value) {
        if (character == '\\' || character == '"') result += '\\';
        if (character == '\n') result += 'n';
        else if (character == '\r') result += 'r';
        else if (character == '\t') result += 't';
        else result += character;
    }
    return result + '"';
}

const char* kind_name(EmulatorEventKind kind) {
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

void optional_hex(std::ostringstream& output, const std::optional<std::uint32_t>& value) {
    if (value) output << json(hex32(*value)); else output << "null";
}

void snapshot(std::ostringstream& output, const EmulatorRegisterSnapshot& value) {
    output << "{\"d\":[";
    for (std::size_t index = 0; index < value.d.size(); ++index) {
        if (index) output << ',';
        optional_hex(output, value.d[index]);
    }
    output << "],\"a\":[";
    for (std::size_t index = 0; index < value.a.size(); ++index) {
        if (index) output << ',';
        optional_hex(output, value.a[index]);
    }
    output << "],\"sr\":";
    if (value.sr) output << json(hex32(*value.sr)); else output << "null";
    output << '}';
}

void event(std::ostringstream& output, const ExternalTraceEvent& value) {
    output << "{\"sequence\":" << value.sequence << ",\"pc\":" << json(hex32(value.pc))
        << ",\"kind\":" << json(kind_name(value.kind)) << ",\"block_start\":";
    optional_hex(output, value.block_start);
    output << ",\"target\":";
    optional_hex(output, value.target);
    output << ",\"taken\":";
    if (value.taken) output << (*value.taken ? "true" : "false"); else output << "null";
    output << ",\"address\":";
    optional_hex(output, value.address);
    output << ",\"width_bytes\":" << static_cast<unsigned>(value.width_bytes)
        << ",\"instruction_size\":" << static_cast<unsigned>(value.instruction_size)
        << ",\"frame\":";
    if (value.frame) output << *value.frame; else output << "null";
    output << ",\"cycles\":";
    if (value.cycles) output << *value.cycles; else output << "null";
    output << ",\"registers\":";
    if (value.registers) snapshot(output, *value.registers); else output << "null";
    output << '}';
}

} // namespace

std::string emulator_trace_to_json(const EmulatorTraceReport& report) {
    std::ostringstream output;
    output << "{\"schema\":\"oasis.m68k.emulator-trace.v1\",\"metadata\":{";
    output << "\"rom_id\":" << json(report.metadata.rom_id)
        << ",\"rom_sha256\":" << json(report.metadata.rom_sha256)
        << ",\"emulator\":" << json(report.metadata.emulator)
        << ",\"version\":" << json(report.metadata.version)
        << ",\"scenario\":" << json(report.metadata.scenario)
        << ",\"event_limit\":" << report.metadata.event_limit << "},\"events\":[";
    for (std::size_t index = 0; index < report.events.size(); ++index) {
        if (index) output << ',';
        event(output, report.events[index]);
    }
    output << "],\"unique_pcs\":[";
    for (std::size_t index = 0; index < report.unique_pcs.size(); ++index) {
        if (index) output << ',';
        output << json(hex32(report.unique_pcs[index]));
    }
    output << "],\"executed_basic_blocks\":[";
    for (std::size_t index = 0; index < report.executed_basic_blocks.size(); ++index) {
        if (index) output << ',';
        output << json(hex32(report.executed_basic_blocks[index]));
    }
    output << "],\"executed_ranges\":[";
    for (std::size_t index = 0; index < report.executed_ranges.size(); ++index) {
        if (index) output << ',';
        const auto& range = report.executed_ranges[index];
        output << "{\"start\":" << json(hex32(range.start)) << ",\"end\":" << json(hex32(range.end))
            << ",\"evidence\":" << json(range.evidence) << '}';
    }
    output << "],\"direct_call_edges\":[";
    for (std::size_t index = 0; index < report.direct_call_edges.size(); ++index) {
        if (index) output << ',';
        const auto& edge = report.direct_call_edges[index];
        output << "{\"caller\":" << json(hex32(edge.caller)) << ",\"callee\":" << json(hex32(edge.callee))
            << ",\"count\":" << edge.count << '}';
    }
    output << "],\"indirect_targets\":[";
    for (std::size_t index = 0; index < report.indirect_targets.size(); ++index) {
        if (index) output << ',';
        output << json(hex32(report.indirect_targets[index]));
    }
    output << "],\"atlas_known_pcs\":[";
    for (std::size_t index = 0; index < report.atlas_known_pcs.size(); ++index) {
        if (index) output << ',';
        output << json(hex32(report.atlas_known_pcs[index]));
    }
    output << "],\"atlas_unknown_pcs\":[";
    for (std::size_t index = 0; index < report.atlas_unknown_pcs.size(); ++index) {
        if (index) output << ',';
        output << json(hex32(report.atlas_unknown_pcs[index]));
    }
    output << "],\"atlas_known_control_flow_targets\":[";
    for (std::size_t index = 0; index < report.atlas_known_control_flow_targets.size(); ++index) {
        if (index) output << ',';
        output << json(hex32(report.atlas_known_control_flow_targets[index]));
    }
    output << "],\"atlas_unknown_control_flow_targets\":[";
    for (std::size_t index = 0; index < report.atlas_unknown_control_flow_targets.size(); ++index) {
        if (index) output << ',';
        output << json(hex32(report.atlas_unknown_control_flow_targets[index]));
    }
    output << "],\"reset_vectors\":";
    if (report.reset_vectors) output << "{\"initial_sp\":" << json(hex32(report.reset_vectors->initial_sp))
        << ",\"initial_pc\":" << json(hex32(report.reset_vectors->initial_pc)) << '}';
    else output << "null";
    output << ",\"first_observed_pc\":";
    optional_hex(output, report.first_observed_pc);
    output << ",\"first_pc_matches_reset\":";
    if (report.first_pc_matches_reset) output << (*report.first_pc_matches_reset ? "true" : "false"); else output << "null";
    output << ",\"deterministic_fields\":[";
    for (std::size_t index = 0; index < report.deterministic_fields.size(); ++index) {
        if (index) output << ',';
        output << json(report.deterministic_fields[index]);
    }
    output << "],\"nondeterministic_fields\":[";
    for (std::size_t index = 0; index < report.nondeterministic_fields.size(); ++index) {
        if (index) output << ',';
        output << json(report.nondeterministic_fields[index]);
    }
    output << "],\"metrics\":{";
    output << "\"branches\":" << report.branch_count << ",\"calls\":" << report.call_count
        << ",\"returns\":" << report.return_count << ",\"memory_reads\":" << report.memory_read_count
        << ",\"memory_writes\":" << report.memory_write_count << ",\"trace_hash\":" << json(report.trace_hash) << "}}";
    return output.str();
}

std::string emulator_trace_to_text(const EmulatorTraceReport& report) {
    std::ostringstream output;
    output << "oasis.m68k.emulator-trace.v1\n"
        << "emulator=" << report.metadata.emulator << " version=" << report.metadata.version
        << " scenario=" << report.metadata.scenario << "\n"
        << "first_pc=" << (report.first_observed_pc ? hex32(*report.first_observed_pc) : "UNKNOWN")
        << " reset_match=" << (report.first_pc_matches_reset ? (*report.first_pc_matches_reset ? "yes" : "no") : "UNKNOWN") << '\n'
        << "events=" << report.events.size() << " unique_pcs=" << report.unique_pcs.size()
        << " blocks=" << report.executed_basic_blocks.size() << " ranges=" << report.executed_ranges.size()
        << " branches=" << report.branch_count << " calls=" << report.call_count
        << " returns=" << report.return_count << " reads=" << report.memory_read_count
        << " writes=" << report.memory_write_count << " hash=" << report.trace_hash << '\n'
        << "atlas_known=" << report.atlas_known_pcs.size() << " atlas_unknown=" << report.atlas_unknown_pcs.size()
        << " known_targets=" << report.atlas_known_control_flow_targets.size()
        << " unknown_targets=" << report.atlas_unknown_control_flow_targets.size()
        << " indirect_targets=" << report.indirect_targets.size() << '\n';
    return output.str();
}

} // namespace oasis::tools
