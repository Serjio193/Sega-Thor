#include "tools/re_explore.hpp"

#include <iomanip>
#include <sstream>

namespace oasis::tools {
namespace {

std::string quote(const std::string& value) {
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

std::string hex32(std::uint32_t value) {
    std::ostringstream out;
    out << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << value;
    return out.str();
}

template <typename T> void addresses(std::ostringstream& out, const std::vector<T>& values) {
    out << '[';
    for (std::size_t i = 0; i < values.size(); ++i) { if (i) out << ','; out << quote(hex32(values[i])); }
    out << ']';
}

void seeds(std::ostringstream& out, const std::vector<ExploreSeed>& values) {
    out << '[';
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i) out << ',';
        const auto& item = values[i];
        out << "{\"address\":" << quote(hex32(item.address)) << ",\"tier\":" << quote(seed_tier_name(item.tier))
            << ",\"priority\":" << item.priority << ",\"source\":" << quote(item.source)
            << ",\"source_reference\":" << quote(item.source_reference)
            << ",\"initial_confidence\":" << quote(item.initial_confidence) << '}';
    }
    out << ']';
}

void bytes(std::ostringstream& out, const std::vector<std::uint8_t>& values) {
    out << '[';
    for (std::size_t i = 0; i < values.size(); ++i) { if (i) out << ','; out << static_cast<unsigned>(values[i]); }
    out << ']';
}

void strings(std::ostringstream& out, const std::vector<std::string>& values) {
    out << '[';
    for (std::size_t i = 0; i < values.size(); ++i) { if (i) out << ','; out << quote(values[i]); }
    out << ']';
}

void json_entry(std::ostringstream& out, const ExploreEntry& item) {
    out << "{\"address\":" << quote(hex32(item.address)) << ",\"state\":" << quote(analysis_state_name(item.state))
        << ",\"seeds\":"; seeds(out, item.seeds);
    out << ",\"decoded_instructions\":" << item.decoded_instruction_count
        << ",\"supported_instruction_bytes\":" << item.supported_instruction_bytes
        << ",\"blocks\":" << item.block_count << ",\"frontier_count\":" << item.frontier_count
        << ",\"stop_count\":" << item.stop_count << '}';
}

void json_map(std::ostringstream& out, const ExploreMapRange& item) {
    out << "{\"start\":" << quote(hex32(item.start)) << ",\"end\":" << quote(hex32(item.end))
        << ",\"type\":" << quote(region_type_name(item.type)) << ",\"owners\":";
    addresses(out, item.owners);
    out << ",\"sources\":"; strings(out, item.sources);
    out << ",\"confidence\":" << quote(item.confidence) << ",\"evidence\":" << quote(item.evidence)
        << ",\"analysis_status\":" << quote(analysis_state_name(item.status)) << '}';
}

void json_frontier(std::ostringstream& out, const FrontierRecord& item) {
    out << "{\"id\":" << quote(item.id) << ",\"source_entry\":" << quote(hex32(item.source_entry))
        << ",\"source_pc\":" << quote(hex32(item.source_pc)) << ",\"blocker_type\":" << quote(frontier_type_name(item.blocker_type))
        << ",\"instruction_bytes\":"; bytes(out, item.instruction_bytes);
    out << ",\"opcode\":" << quote(hex32(item.opcode)) << ",\"instruction\":" << quote(item.instruction)
        << ",\"known_context\":" << quote(item.known_context) << ",\"owners\":"; addresses(out, item.owners);
    out << ",\"evidence\":"; strings(out, item.evidence);
    out << ",\"stop_reason\":" << quote(stop_reason_name(item.stop_reason)) << ",\"reason\":" << quote(item.reason) << '}';
}

void json_metrics(std::ostringstream& out, const ExploreMetrics& m) {
    out << "{\"total_rom_bytes\":" << m.total_rom_bytes << ",\"instruction_bytes\":" << m.instruction_bytes
        << ",\"known_data_bytes\":" << m.known_data_bytes << ",\"probable_data_bytes\":" << m.probable_data_bytes
        << ",\"pointer_data_bytes\":" << m.pointer_data_bytes << ",\"conflict_bytes\":" << m.conflict_bytes
        << ",\"unclassified_bytes\":" << m.unclassified_bytes << ",\"seeds\":" << m.seeds
        << ",\"discovered_entries\":" << m.discovered_entries << ",\"queued_entries\":" << m.queued_entries
        << ",\"analyzed_entries\":" << m.analyzed_entries << ",\"blocked_indirect\":" << m.blocked_indirect_entries
        << ",\"blocked_unsupported\":" << m.blocked_unsupported_entries << ",\"blocked_data\":" << m.blocked_data_entries
        << ",\"conflict_entries\":" << m.conflict_entries << ",\"direct_calls\":" << m.direct_calls
        << ",\"direct_jumps\":" << m.direct_jumps << ",\"conditional_branches\":" << m.conditional_branches
        << ",\"fallthroughs\":" << m.fallthroughs << ",\"unresolved_indirect\":" << m.unresolved_indirect
        << ",\"decoded_instructions\":" << m.decoded_instructions << ",\"unsupported_instructions\":" << m.unsupported_instructions
        << ",\"decode_failures\":" << m.decode_failures << ",\"frontier_count\":" << m.frontier_count
        << ",\"entries_processed\":" << m.entries_processed
        << ",\"dynamic_indirect_edges\":" << m.dynamic_indirect_edges << '}';
}

} // namespace

std::string explore_to_json(const ExploreReport& report) {
    std::ostringstream out;
    out << "{\"schema\":" << quote(report.schema) << ",\"rom_identity\":" << quote(report.rom_identity)
        << ",\"bounded_control_pass\":" << (report.bounded_control_pass ? "true" : "false")
        << ",\"rom_wide_requested\":" << (report.rom_wide_requested ? "true" : "false")
        << ",\"rom_wide_performed\":" << (report.rom_wide_performed ? "true" : "false")
        << ",\"rom_wide_skip_reason\":" << quote(report.rom_wide_skip_reason) << ",\"seeds\":";
    seeds(out, report.seeds);
    out << ",\"processing_order\":"; addresses(out, report.processing_order);
    out << ",\"entries\":[";
    for (std::size_t i = 0; i < report.entries.size(); ++i) { if (i) out << ','; json_entry(out, report.entries[i]); }
    out << "],\"edges\":[";
    for (std::size_t i = 0; i < report.edges.size(); ++i) {
        if (i) out << ',';
        const auto& item = report.edges[i];
        out << "{\"source_entry\":" << quote(hex32(item.source_entry)) << ",\"source_pc\":" << quote(hex32(item.source_pc))
            << ",\"target\":" << quote(hex32(item.target)) << ",\"kind\":" << quote(explore_edge_name(item.kind))
            << ",\"target_queued\":" << (item.target_queued ? "true" : "false")
            << ",\"evidence_class\":" << quote(item.evidence_class)
            << ",\"frontier_id\":" << quote(item.frontier_id)
            << ",\"job_id\":" << quote(item.job_id)
            << ",\"result_hash\":" << quote(item.result_hash)
            << ",\"backend\":" << quote(item.backend)
            << ",\"scenario\":" << quote(item.scenario) << '}';
    }
    out << "],\"stops\":[";
    for (std::size_t i = 0; i < report.stops.size(); ++i) { if (i) out << ','; const auto& item = report.stops[i]; out << "{\"source_entry\":" << quote(hex32(item.source_entry)) << ",\"source_pc\":" << quote(hex32(item.source_pc)) << ",\"reason\":" << quote(stop_reason_name(item.reason)) << ",\"detail\":" << quote(item.detail) << '}'; }
    out << "],\"frontier\":[";
    for (std::size_t i = 0; i < report.frontier.size(); ++i) { if (i) out << ','; json_frontier(out, report.frontier[i]); }
    out << "],\"address_map\":[";
    for (std::size_t i = 0; i < report.address_map.size(); ++i) { if (i) out << ','; json_map(out, report.address_map[i]); }
    out << "],\"control_set\":[";
    for (std::size_t i = 0; i < report.control_set.size(); ++i) {
        if (i) out << ',';
        const auto& item = report.control_set[i];
        out << "{\"entry\":" << quote(hex32(item.entry)) << ",\"present\":" << (item.present ? "true" : "false")
            << ",\"analyzed\":" << (item.analyzed ? "true" : "false") << ",\"heuristic_pass\":" << (item.heuristic_pass ? "true" : "false")
            << ",\"heuristic_miss\":" << (item.heuristic_miss ? "true" : "false") << ",\"recovered_edges\":[";
        for (std::size_t j = 0; j < item.recovered_edges.size(); ++j) { if (j) out << ','; out << "{\"source\":" << quote(hex32(item.recovered_edges[j].source)) << ",\"target\":" << quote(hex32(item.recovered_edges[j].target)) << '}'; }
        out << "]}";
    }
    out << "],\"blocker_clusters\":[";
    for (std::size_t i = 0; i < report.blocker_clusters.size(); ++i) { if (i) out << ','; const auto& item = report.blocker_clusters[i]; out << "{\"blocker_type\":" << quote(item.blocker_type) << ",\"count\":" << item.count << ",\"representative_addresses\":"; addresses(out, item.representative_addresses); out << '}'; }
    out << "],\"metrics\":"; json_metrics(out, report.metrics); out << '}';
    return out.str();
}

std::string explore_to_text(const ExploreReport& report) {
    const auto& m = report.metrics;
    std::ostringstream out;
    out << report.schema << '\n'
        << "rom_identity=" << report.rom_identity << " bounded_control_pass=" << (report.bounded_control_pass ? "yes" : "no")
        << " rom_wide=" << (report.rom_wide_performed ? "performed" : report.rom_wide_requested ? "skipped" : "not_requested") << '\n'
        << "coverage total=" << m.total_rom_bytes << " instruction=" << m.instruction_bytes << " known_data=" << m.known_data_bytes
        << " probable_data=" << m.probable_data_bytes << " pointer_data=" << m.pointer_data_bytes << " conflict=" << m.conflict_bytes
        << " unclassified=" << m.unclassified_bytes << " (" << std::fixed << std::setprecision(2)
        << (m.total_rom_bytes ? 100.0 * m.unclassified_bytes / m.total_rom_bytes : 0.0) << "%)\n"
        << "entries seeds=" << m.seeds << " discovered=" << m.discovered_entries << " queued=" << m.queued_entries
        << " analyzed=" << m.analyzed_entries << " blocked_indirect=" << m.blocked_indirect_entries
        << " blocked_unsupported=" << m.blocked_unsupported_entries << " blocked_data=" << m.blocked_data_entries
        << " conflicts=" << m.conflict_entries << " processed=" << m.entries_processed << '\n'
        << "edges calls=" << m.direct_calls << " jumps=" << m.direct_jumps << " conditional=" << m.conditional_branches
        << " fallthrough=" << m.fallthroughs << " unresolved_indirect=" << m.unresolved_indirect
        << " dynamic_indirect=" << m.dynamic_indirect_edges << '\n'
        << "instructions decoded=" << m.decoded_instructions << " unsupported=" << m.unsupported_instructions
        << " decode_failures=" << m.decode_failures << " frontier=" << m.frontier_count << " wall_clock_ms=CLI_ONLY\n"
        << "control_set:\n";
    for (const auto& item : report.control_set) out << "- " << hex32(item.entry) << " present=" << (item.present ? "yes" : "no") << " analyzed=" << (item.analyzed ? "yes" : "no") << " heuristic=" << (item.heuristic_pass ? "PASS" : "MISS") << " recovered_edges=" << item.recovered_edges.size() << '\n';
    out << "blocker_clusters:\n";
    for (const auto& item : report.blocker_clusters) { out << "- " << item.blocker_type << " count=" << item.count << " representatives="; for (std::size_t i = 0; i < item.representative_addresses.size(); ++i) { if (i) out << ','; out << hex32(item.representative_addresses[i]); } out << '\n'; }
    out << "address_map_ranges=" << report.address_map.size() << '\n';
    return out.str();
}

} // namespace oasis::tools
