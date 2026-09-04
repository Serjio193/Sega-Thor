#include "tools/re_cfg_audit.hpp"

#include <iomanip>
#include <sstream>

namespace oasis::tools {
namespace {

std::string hex32(std::uint32_t value) {
    std::ostringstream out;
    out << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << value;
    return out.str();
}

std::string hex_bytes(const std::vector<std::uint8_t>& bytes) {
    std::ostringstream out;
    for (const auto byte : bytes)
        out << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << static_cast<unsigned>(byte);
    return out.str();
}

std::string json_string(const std::string& value) {
    std::string result = "\"";
    for (const auto character : value) {
        if (character == '\\' || character == '"') result += '\\';
        if (character == '\n') result += "n";
        else if (character == '\r') result += "r";
        else if (character == '\t') result += "t";
        else result += character;
    }
    return result + '"';
}

std::string memory_kind(MemoryKind kind) {
    if (kind == MemoryKind::rom) return "rom";
    if (kind == MemoryKind::ram) return "ram";
    return "other";
}

std::string memory_access(MemoryAccess access) {
    if (access == MemoryAccess::read) return "read";
    if (access == MemoryAccess::write) return "write";
    if (access == MemoryAccess::address) return "address";
    return "unknown";
}

void json_edge(std::ostringstream& out, const AuditIncomingEdge& edge) {
    out << "{\"source\":" << json_string(hex32(edge.source)) << ",\"target\":" << json_string(hex32(edge.target))
        << ",\"source_function\":" << json_string(hex32(edge.source_function))
        << ",\"kind\":" << json_string(edge.kind) << '}';
}

void json_edges(std::ostringstream& out, const std::vector<AuditIncomingEdge>& edges) {
    out << '[';
    for (std::size_t i = 0; i < edges.size(); ++i) { if (i) out << ','; json_edge(out, edges[i]); }
    out << ']';
}

void json_addresses(std::ostringstream& out, const std::vector<std::uint32_t>& addresses) {
    out << '[';
    for (std::size_t i = 0; i < addresses.size(); ++i) { if (i) out << ','; out << json_string(hex32(addresses[i])); }
    out << ']';
}

void json_record(std::ostringstream& out, const AuditRecord& record) {
    out << "{\"instruction_address\":" << json_string(hex32(record.instruction_address))
        << ",\"byte_end\":" << json_string(hex32(record.byte_end))
        << ",\"block_start\":" << json_string(hex32(record.block_start))
        << ",\"opcode\":" << json_string(hex32(record.opcode))
        << ",\"mnemonic\":" << json_string(record.mnemonic)
        << ",\"decoder_status\":" << json_string(record.decoder_status)
        << ",\"bytes\":" << json_string(hex_bytes(record.bytes))
        << ",\"direct_memory_references\":[";
    for (std::size_t i = 0; i < record.direct_memory_references.size(); ++i) {
        if (i) out << ',';
        const auto& ref = record.direct_memory_references[i];
        out << "{\"address\":" << json_string(hex32(ref.address)) << ",\"width_bytes\":" << static_cast<unsigned>(ref.width_bytes)
            << ",\"kind\":" << json_string(memory_kind(ref.kind)) << ",\"access\":" << json_string(memory_access(ref.access)) << '}';
    }
    out << "],\"unresolved_memory_references\":[";
    for (std::size_t i = 0; i < record.unresolved_memory_references.size(); ++i) {
        if (i) out << ',';
        const auto& ref = record.unresolved_memory_references[i];
        out << "{\"mode\":" << static_cast<unsigned>(ref.mode) << ",\"register\":" << static_cast<unsigned>(ref.register_index)
            << ",\"reason\":" << json_string(ref.reason) << '}';
    }
    out << "],\"nearest_preceding_reachable\":";
    if (record.nearest_preceding_reachable) out << json_string(hex32(*record.nearest_preceding_reachable)); else out << "null";
    out << ",\"nearest_following_reachable\":";
    if (record.nearest_following_reachable) out << json_string(hex32(*record.nearest_following_reachable)); else out << "null";
    out << ",\"nearest_preceding_reachable_block\":";
    if (record.nearest_preceding_reachable_block) out << json_string(hex32(*record.nearest_preceding_reachable_block)); else out << "null";
    out << ",\"nearest_following_reachable_block\":";
    if (record.nearest_following_reachable_block) out << json_string(hex32(*record.nearest_following_reachable_block)); else out << "null";
    out << ",\"preceding_distance\":" << record.preceding_distance << ",\"following_distance\":" << record.following_distance
        << ",\"incoming_edges\":"; json_edges(out, record.incoming_edges);
    out << ",\"outgoing_targets\":"; json_addresses(out, record.outgoing_targets);
    out << ",\"fallthrough_possible\":" << (record.fallthrough_possible ? "true" : "false")
        << ",\"alignment_padding_pattern\":" << (record.alignment_padding_pattern ? "true" : "false")
        << ",\"embedded_data_pattern\":" << (record.embedded_data_pattern ? "true" : "false")
        << ",\"decoder_supported\":" << (record.decoder_supported ? "true" : "false")
        << ",\"classification\":" << json_string(cfg_audit_classification_name(record.classification))
        << ",\"confidence\":" << json_string(record.confidence)
        << ",\"reason\":" << json_string(record.reason) << '}';
}

} // namespace

std::string cfg_audit_to_json(const CfgAuditReport& report) {
    std::ostringstream out;
    out << "{\"schema\":\"oasis.m68k.re-cfg-audit.v1\",\"target_entry\":" << json_string(hex32(report.target_entry))
        << ",\"window\":{\"start\":" << json_string(hex32(report.window_start)) << ",\"end\":" << json_string(hex32(report.window_end)) << "}"
        << ",\"metrics\":{\"raw_static_evidence_records\":" << report.raw_static_evidence_records
        << ",\"outside_reachable_records\":" << report.outside_reachable_records
        << ",\"reachable_unresolved_after_resolution\":" << report.reachable_unresolved_after_resolution
        << ",\"nonreachable_unresolved\":" << report.nonreachable_unresolved
        << ",\"raw_unresolved_after_resolution\":" << report.raw_unresolved_after_resolution
        << ",\"records_with_known_incoming_edges\":" << report.records_with_known_incoming_edges
        << ",\"records_without_known_incoming_edges\":" << report.records_without_known_incoming_edges
        << ",\"secondary_entry_candidates\":" << report.secondary_entry_candidates
        << ",\"suspected_data_or_artifact_records\":" << report.suspected_data_or_artifact_records
        << ",\"unknown_remainder\":" << report.unknown_remainder
        << ",\"atlas_unresolved_before\":" << report.atlas_unresolved_before
        << ",\"atlas_unresolved_after_audit\":" << report.atlas_unresolved_after_audit
        << ",\"ranking_displacement_before\":" << report.ranking_displacement_before
        << ",\"ranking_displacement_after_audit\":" << report.ranking_displacement_after_audit << "}"
        << ",\"beta_evidence\":" << json_string(report.beta_evidence) << ",\"reachability_factors\":[";
    for (std::size_t i = 0; i < report.reachability_factors.size(); ++i) {
        if (i) out << ',';
        const auto& item = report.reachability_factors[i];
        out << "{\"factor\":" << json_string(item.key) << ",\"records\":" << item.records << '}';
    }
    out << "],\"classification_counts\":[";
    for (std::size_t i = 0; i < report.classification_counts.size(); ++i) {
        if (i) out << ',';
        const auto& item = report.classification_counts[i];
        out << "{\"classification\":" << json_string(item.key) << ",\"records\":" << item.records << ",\"bytes\":" << item.bytes << '}';
    }
    out << "],\"islands\":[";
    for (std::size_t i = 0; i < report.islands.size(); ++i) {
        if (i) out << ',';
        const auto& island = report.islands[i];
        out << "{\"id\":" << json_string(island.id) << ",\"start\":" << json_string(hex32(island.start))
            << ",\"end\":" << json_string(hex32(island.end)) << ",\"byte_count\":" << island.byte_count
            << ",\"instruction_count\":" << island.instruction_count << ",\"record_addresses\":";
        json_addresses(out, island.record_addresses);
        out << ",\"incoming_edges\":"; json_edges(out, island.incoming_edges);
        out << ",\"outgoing_targets\":"; json_addresses(out, island.outgoing_targets);
        out << ",\"terminating_instruction\":";
        if (island.terminating_instruction) out << json_string(hex32(*island.terminating_instruction)); else out << "null";
        out << ",\"classification\":" << json_string(cfg_audit_classification_name(island.classification))
            << ",\"confidence\":" << json_string(island.confidence) << '}';
    }
    out << "],\"records\":[";
    for (std::size_t i = 0; i < report.records.size(); ++i) { if (i) out << ','; json_record(out, report.records[i]); }
    return out.str() + "]}";
}

std::string cfg_audit_to_text(const CfgAuditReport& report) {
    std::ostringstream out;
    out << "oasis.m68k.re-cfg-audit.v1\n"
        << "target=" << hex32(report.target_entry) << " window=" << hex32(report.window_start) << ".." << hex32(report.window_end)
        << " raw=" << report.raw_static_evidence_records << " outside=" << report.outside_reachable_records
        << " reachable_unresolved=" << report.reachable_unresolved_after_resolution
        << " nonreachable_unresolved=" << report.nonreachable_unresolved << " raw_after=" << report.raw_unresolved_after_resolution << '\n'
        << "incoming=" << report.records_with_known_incoming_edges << " none=" << report.records_without_known_incoming_edges
        << " secondary=" << report.secondary_entry_candidates << " data_or_artifact=" << report.suspected_data_or_artifact_records
        << " unknown=" << report.unknown_remainder << " atlas=" << report.atlas_unresolved_before << " -> " << report.atlas_unresolved_after_audit << '\n';
    out << "factors:";
    for (const auto& item : report.reachability_factors) out << ' ' << item.key << '=' << item.records;
    out << "\nclassifications:";
    for (const auto& item : report.classification_counts) out << ' ' << item.key << '=' << item.records << '/' << item.bytes;
    out << "\nislands=" << report.islands.size() << '\n';
    for (const auto& island : report.islands)
        out << "- " << island.id << ' ' << hex32(island.start) << ".." << hex32(island.end)
            << " records=" << island.instruction_count << " class=" << cfg_audit_classification_name(island.classification)
            << " incoming=" << island.incoming_edges.size() << " outgoing=" << island.outgoing_targets.size() << '\n';
    out << "records:\n";
    for (const auto& record : report.records)
        out << "- " << hex32(record.instruction_address) << ".." << hex32(record.byte_end)
            << " block=" << hex32(record.block_start) << " opcode=" << hex32(record.opcode)
            << " mnemonic=" << record.mnemonic
            << " class=" << cfg_audit_classification_name(record.classification) << ':' << record.confidence
            << " prev=" << (record.nearest_preceding_reachable ? hex32(*record.nearest_preceding_reachable) : "none")
            << " next=" << (record.nearest_following_reachable ? hex32(*record.nearest_following_reachable) : "none")
            << " incoming=" << record.incoming_edges.size() << " bytes=" << hex_bytes(record.bytes) << '\n';
    return out.str();
}

} // namespace oasis::tools
