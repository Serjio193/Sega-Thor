#include "tools/re_atlas.hpp"

#include "tools/re_slice_decoder.hpp"

#include <iomanip>
#include <sstream>

namespace oasis::tools {
namespace {

std::string hex32(std::uint32_t value) {
    std::ostringstream out;
    out << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << value;
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
    result += '"';
    return result;
}

template <typename T>
void json_numbers(std::ostringstream& out, const std::vector<T>& values) {
    out << '[';
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i) out << ',';
        out << json_string(hex32(values[i]));
    }
    out << ']';
}

void json_strings(std::ostringstream& out, const std::vector<std::string>& values) {
    out << '[';
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i) out << ',';
        out << json_string(values[i]);
    }
    out << ']';
}

void json_identity(std::ostringstream& out, const RomIdentity& identity) {
    out << "{\"id\":" << json_string(identity.id)
        << ",\"display_name\":" << json_string(identity.display_name)
        << ",\"status\":" << json_string(std::string(to_string(identity.status)))
        << ",\"size\":" << identity.fingerprint.size
        << ",\"crc32\":" << json_string(hex32(identity.fingerprint.crc32))
        << ",\"sha1\":" << json_string(identity.fingerprint.sha1)
        << ",\"sha256\":" << json_string(identity.fingerprint.sha256)
        << ",\"sega_checksum_valid\":" << (identity.fingerprint.sega_checksum_valid ? "true" : "false")
        << '}';
}

void json_entry(std::ostringstream& out, const AtlasEntry& entry) {
    out << "{\"id\":" << json_string(entry.id)
        << ",\"type\":" << json_string(atlas_entry_type_name(entry.type))
        << ",\"start\":" << json_string(hex32(entry.start))
        << ",\"end\":";
    if (entry.end) out << json_string(hex32(*entry.end)); else out << "null";
    out << ",\"bounded_evidence_end\":";
    if (entry.bounded_evidence_end) out << json_string(hex32(*entry.bounded_evidence_end)); else out << "null";
    out << ",\"semantic_confidence\":" << json_string(atlas_confidence_name(entry.semantic_confidence))
        << ",\"boundary_confidence\":" << json_string(atlas_confidence_name(entry.boundary_confidence))
        << ",\"evidence_sources\":";
    json_strings(out, entry.evidence_sources);
    out << ",\"callers\":"; json_numbers(out, entry.callers);
    out << ",\"callees\":"; json_numbers(out, entry.callees);
    out << ",\"direct_rom_refs\":"; json_numbers(out, entry.direct_rom_refs);
    out << ",\"direct_ram_refs\":"; json_numbers(out, entry.direct_ram_refs);
    out << ",\"unresolved_reference_count\":" << entry.unresolved_reference_count
        << ",\"unsupported_evidence_count\":" << entry.unsupported_evidence_count
        << ",\"indirect_control_flow_count\":" << entry.indirect_control_flow_count
        << ",\"beta\":";
    if (!entry.beta) out << "null";
    else {
        out << "{\"address\":" << json_string(hex32(entry.beta->address))
            << ",\"match\":" << json_string(atlas_correspondence_name(entry.beta->match))
            << ",\"changed_blocks\":[";
        for (std::size_t i = 0; i < entry.beta->changed_blocks.size(); ++i) {
            if (i) out << ',';
            out << entry.beta->changed_blocks[i];
        }
        out << "]}";
    }
    out << ",\"dynamic\":";
    if (!entry.dynamic) out << "null";
    else {
        const auto& dynamic = *entry.dynamic;
        out << "{\"executed_instructions\":" << dynamic.executed_instructions
            << ",\"executed_basic_blocks\":" << dynamic.executed_basic_blocks
            << ",\"memory_reads\":" << dynamic.memory_reads
            << ",\"memory_writes\":" << dynamic.memory_writes
            << ",\"branches\":" << dynamic.branches
            << ",\"calls\":" << dynamic.calls
            << ",\"returns\":" << dynamic.returns << ",\"raw_facts\":";
        json_strings(out, dynamic.raw_facts);
        out << '}';
    }
    out << ",\"native\":{\"status\":" << json_string(native_status_name(entry.native.status))
        << ",\"path\":";
    if (entry.native.path) out << json_string(*entry.native.path); else out << "null";
    out << "},\"verification_status\":" << json_string(entry.verification_status)
        << ",\"notes\":" << json_string(entry.notes) << '}';
}

} // namespace

std::string atlas_entry_type_name(AtlasEntryType type) {
    switch (type) {
    case AtlasEntryType::function: return "function";
    case AtlasEntryType::bounded_code: return "bounded_code";
    case AtlasEntryType::data: return "data";
    case AtlasEntryType::table: return "table";
    case AtlasEntryType::unknown_ref: return "unknown_ref";
    }
    return "unknown";
}

std::string atlas_confidence_name(AtlasConfidence confidence) {
    switch (confidence) {
    case AtlasConfidence::confirmed: return "CONFIRMED";
    case AtlasConfidence::likely: return "LIKELY";
    case AtlasConfidence::hypothesis: return "HYPOTHESIS";
    case AtlasConfidence::unknown: return "UNKNOWN";
    }
    return "UNKNOWN";
}

std::string atlas_correspondence_name(AtlasCorrespondence match) {
    switch (match) {
    case AtlasCorrespondence::exact: return "exact";
    case AtlasCorrespondence::structural: return "structural";
    case AtlasCorrespondence::changed: return "changed";
    case AtlasCorrespondence::unmatched: return "unmatched";
    case AtlasCorrespondence::not_checked: return "not_checked";
    }
    return "not_checked";
}

std::string native_status_name(NativeStatus status) {
    switch (status) {
    case NativeStatus::verified: return "verified";
    case NativeStatus::present_unverified: return "present_unverified";
    case NativeStatus::not_applicable: return "not_applicable";
    case NativeStatus::not_implemented: return "not_implemented";
    }
    return "not_implemented";
}

std::string atlas_to_json(const AtlasReport& report) {
    std::ostringstream out;
    out << "{\"schema\":\"oasis.m68k.re-atlas.v1\",\"retail\":";
    json_identity(out, report.retail);
    out << ",\"beta\":";
    if (report.beta) json_identity(out, *report.beta); else out << "null";
    out << ",\"entries\":[";
    for (std::size_t i = 0; i < report.entries.size(); ++i) {
        if (i) out << ',';
        json_entry(out, report.entries[i]);
    }
    out << "],\"call_graph\":[";
    for (std::size_t i = 0; i < report.call_edges.size(); ++i) {
        if (i) out << ',';
        const auto& edge = report.call_edges[i];
        out << "{\"caller\":" << json_string(hex32(edge.caller)) << ",\"callee\":" << json_string(hex32(edge.callee))
            << ",\"call_sites\":";
        json_numbers(out, edge.call_sites);
        out << '}';
    }
    out << "],\"coverage\":{\"rom_size\":" << report.coverage.rom_size
        << ",\"confirmed_classified_bytes\":" << report.coverage.confirmed_classified_bytes
        << ",\"bounded_evidence_bytes\":" << report.coverage.bounded_evidence_bytes
        << ",\"overlapping_evidence_bytes\":" << report.coverage.overlapping_evidence_bytes
        << ",\"unknown_remainder_bytes\":" << report.coverage.unknown_remainder_bytes
        << ",\"atlas_entries\":" << report.coverage.atlas_entries
        << ",\"verified_native_implementations\":" << report.coverage.verified_native_implementations
        << ",\"unresolved_unknown_references\":" << report.coverage.unresolved_unknown_references << "},\"conflicts\":[";
    for (std::size_t i = 0; i < report.conflicts.size(); ++i) {
        if (i) out << ',';
        const auto& conflict = report.conflicts[i];
        out << "{\"left_id\":" << json_string(conflict.left_id)
            << ",\"right_id\":" << json_string(conflict.right_id)
            << ",\"overlap_start\":" << json_string(hex32(conflict.overlap_start))
            << ",\"overlap_end\":" << json_string(hex32(conflict.overlap_end))
            << ",\"reason\":" << json_string(conflict.reason) << '}';
    }
    out << "]}";
    return out.str();
}

std::string atlas_to_text(const AtlasReport& report) {
    std::ostringstream out;
    out << "oasis.m68k.re-atlas.v1\nretail=" << report.retail.display_name
        << " size=" << report.coverage.rom_size << "\n"
        << "coverage confirmed=" << report.coverage.confirmed_classified_bytes
        << " bounded_evidence=" << report.coverage.bounded_evidence_bytes
        << " unknown_remainder=" << report.coverage.unknown_remainder_bytes
        << " overlaps=" << report.coverage.overlapping_evidence_bytes << "\n";
    if (report.beta) out << "beta=" << report.beta->display_name << "\n";
    out << "entries=" << report.entries.size() << " calls=" << report.call_edges.size()
        << " conflicts=" << report.conflicts.size() << "\n";
    for (const auto& entry : report.entries) {
        out << "- " << entry.id << ' ' << hex32(entry.start);
        if (entry.end) out << ".." << hex32(*entry.end);
        else if (entry.bounded_evidence_end) out << " bounded.." << hex32(*entry.bounded_evidence_end);
        out << " type=" << atlas_entry_type_name(entry.type)
            << " boundary=" << atlas_confidence_name(entry.boundary_confidence)
            << " unresolved=" << entry.unresolved_reference_count
            << " unsupported=" << entry.unsupported_evidence_count
            << " indirect=" << entry.indirect_control_flow_count;
        if (entry.beta) out << " beta=" << hex32(entry.beta->address) << ':' << atlas_correspondence_name(entry.beta->match);
        if (entry.dynamic) out << " dynamic_pc=" << entry.dynamic->executed_instructions;
        out << "\n";
    }
    for (const auto& edge : report.call_edges) {
        out << "call " << hex32(edge.caller) << " -> " << hex32(edge.callee) << " sites=";
        for (const auto site : edge.call_sites) out << hex32(site) << ' ';
        out << "\n";
    }
    for (const auto& conflict : report.conflicts)
        out << "CONFLICT " << conflict.left_id << " / " << conflict.right_id << " "
            << hex32(conflict.overlap_start) << ".." << hex32(conflict.overlap_end) << "\n";
    return out.str();
}

} // namespace oasis::tools
