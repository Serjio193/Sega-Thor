#include "tools/re_candidate_map.hpp"

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
    for (const char character : value) {
        if (character == '\\' || character == '"') result += '\\';
        if (character == '\n') result += 'n';
        else if (character == '\r') result += 'r';
        else if (character == '\t') result += 't';
        else result += character;
    }
    result += '"';
    return result;
}

void json_addresses(std::ostringstream& out, const std::vector<std::uint32_t>& values) {
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

void json_optional_address(std::ostringstream& out, const std::optional<std::uint32_t>& value) {
    if (value) out << json_string(hex32(*value));
    else out << "null";
}

void json_candidate(std::ostringstream& out, const CandidateRecord& item) {
    out << "{\"entry\":" << json_string(hex32(item.entry)) << ",\"source_flags\":";
    json_strings(out, item.source_flags);
    out << ",\"ghidra_function\":" << (item.ghidra_function ? "true" : "false")
        << ",\"ghidra_range\":{\"start\":";
    json_optional_address(out, item.ghidra_range_start);
    out << ",\"end\":";
    json_optional_address(out, item.ghidra_range_end);
    out << "},\"ghidra_calls\":";
    json_addresses(out, item.ghidra_calls);
    out << ",\"ghidra_called_by\":";
    json_addresses(out, item.ghidra_called_by);
    out << ",\"atlas_present\":" << (item.atlas_present ? "true" : "false")
        << ",\"atlas_status\":" << json_string(item.atlas_status)
        << ",\"dynamic_observed\":" << (item.dynamic_observed ? "true" : "false")
        << ",\"dynamic_sources\":";
    json_strings(out, item.dynamic_sources);
    out << ",\"beta_match_kind\":" << json_string(item.beta_match_kind)
        << ",\"beta_address\":";
    json_optional_address(out, item.beta_address);
    out << ",\"known_direct_call_target\":" << (item.known_direct_call_target ? "true" : "false")
        << ",\"known_direct_caller\":" << (item.known_direct_caller ? "true" : "false")
        << ",\"known_direct_call_sites\":";
    json_addresses(out, item.known_direct_call_sites);
    out << ",\"code_data_conflict\":" << (item.code_data_conflict ? "true" : "false")
        << ",\"boundary_conflict\":" << (item.boundary_conflict ? "true" : "false")
        << ",\"indirect_flow\":" << (item.indirect_flow ? "true" : "false")
        << ",\"complexity\":" << json_string(candidate_complexity_name(item.complexity))
        << ",\"classification\":" << json_string(candidate_classification_name(item.classification))
        << ",\"ranking_score\":" << item.ranking_score << ",\"ranking_reasons\":";
    json_strings(out, item.ranking_reasons);
    out << ",\"quality_audit\":" << json_string(item.quality_audit) << '}';
}

} // namespace

std::string candidate_map_to_json(const CandidateMapReport& report) {
    std::ostringstream out;
    out << "{\"schema\":" << json_string(report.schema)
        << ",\"ghidra\":{\"schema\":" << json_string(report.ghidra_schema)
        << ",\"program\":" << json_string(report.ghidra_program)
        << ",\"language\":" << json_string(report.ghidra_language)
        << ",\"analysis_mode\":" << json_string(report.ghidra_analysis_mode)
        << ",\"semantic_status\":" << json_string(report.ghidra_semantic_status) << "}"
        << ",\"counts\":{\"total_unique_entries\":" << report.candidates.size()
        << ",\"CONFIRMED\":" << report.confirmed_count
        << ",\"STATIC_SUPPORTED\":" << report.static_supported_count
        << ",\"DYNAMIC_OBSERVED\":" << report.dynamic_observed_count
        << ",\"GHIDRA_ONLY\":" << report.ghidra_only_count
        << ",\"CONFLICT\":" << report.conflict_count << '}'
        << ",\"complexity_counts\":{\"LEAF\":" << report.leaf_count
        << ",\"SHALLOW\":" << report.shallow_count
        << ",\"COMPLEX\":" << report.complex_count
        << ",\"UNKNOWN\":" << report.unknown_complexity_count << '}'
        << ",\"recommended_next_target\":";
    json_optional_address(out, report.recommended_next_target);
    out << ",\"recommended_reason\":" << json_string(report.recommended_reason)
        << ",\"scoring_rules\":[" << json_string("dynamic observed +40")
        << ',' << json_string("known direct call target +20")
        << ',' << json_string("known direct caller/call site +10")
        << ',' << json_string("beta exact +10, structural +8, changed +4")
        << ',' << json_string("leaf +10, shallow +6, complex -8")
        << ',' << json_string("no observed indirect flow +4")
        << ',' << json_string("Atlas entry +5")
        << ',' << json_string("boundary/code-data conflict -50")
        << ',' << json_string("Ghidra-only without xrefs -5") << ']'
        << ",\"candidates\":[";
    for (std::size_t i = 0; i < report.candidates.size(); ++i) {
        if (i) out << ',';
        json_candidate(out, report.candidates[i]);
    }
    out << "]}";
    return out.str();
}

std::string candidate_map_to_text(const CandidateMapReport& report, std::size_t top_n) {
    std::ostringstream out;
    out << "oasis.m68k.re-candidate-map.v1\n"
        << "ghidra=" << report.ghidra_program << " schema=" << report.ghidra_schema
        << " language=" << report.ghidra_language << " analysis=" << report.ghidra_analysis_mode << "\n"
        << "entries=" << report.candidates.size() << " CONFIRMED=" << report.confirmed_count
        << " STATIC_SUPPORTED=" << report.static_supported_count
        << " DYNAMIC_OBSERVED=" << report.dynamic_observed_count
        << " GHIDRA_ONLY=" << report.ghidra_only_count << " CONFLICT=" << report.conflict_count << "\n"
        << "complexity LEAF=" << report.leaf_count << " SHALLOW=" << report.shallow_count
        << " COMPLEX=" << report.complex_count << " UNKNOWN=" << report.unknown_complexity_count << "\n"
        << "recommended_next_target=";
    if (report.recommended_next_target) out << hex32(*report.recommended_next_target);
    else out << "none";
    out << " reason=" << report.recommended_reason << "\n"
        << "score: dynamic +40; direct target +20; direct caller +10; beta exact/structural/changed +10/+8/+4;"
        << " leaf/shallow/complex +10/+6/-8; no indirect +4; Atlas entry +5; conflict -50; no-xref Ghidra-only -5\n"
        << "top_ranked_candidates:\n";
    const auto count = std::min(top_n, report.candidates.size());
    for (std::size_t i = 0; i < count; ++i) {
        const auto& item = report.candidates[i];
        out << (i + 1) << ". " << hex32(item.entry) << " score=" << item.ranking_score
            << " class=" << candidate_classification_name(item.classification)
            << " dynamic=" << (item.dynamic_observed ? "yes" : "no")
            << " complexity=" << candidate_complexity_name(item.complexity)
            << " atlas=" << item.atlas_status << " beta=" << item.beta_match_kind
            << " calls=" << item.ghidra_calls.size() << " callers=" << item.ghidra_called_by.size()
            << " conflict=" << ((item.code_data_conflict || item.boundary_conflict) ? "yes" : "no") << "\n";
        out << "   reasons=";
        for (std::size_t reason = 0; reason < item.ranking_reasons.size(); ++reason) {
            if (reason) out << "; ";
            out << item.ranking_reasons[reason];
        }
        out << "\n";
    }
    out << "top_10_non_confirmed_quality_audit:\n";
    std::size_t audited{};
    for (const auto& item : report.candidates) {
        if (item.classification == CandidateClassification::confirmed || item.quality_audit.empty()) continue;
        out << "- " << hex32(item.entry) << " score=" << item.ranking_score
            << " class=" << candidate_classification_name(item.classification)
            << " audit=" << item.quality_audit << " reason=";
        if (item.code_data_conflict) out << "Ghidra range overlaps an Atlas table";
        else if (item.boundary_conflict) out << "Ghidra boundary differs from confirmed Atlas range";
        else if (item.dynamic_observed) out << "existing runtime observation is available";
        else if (item.known_direct_call_target || item.known_direct_caller) out << "existing static edge/call-site evidence";
        else if (item.ghidra_calls.empty() && item.ghidra_called_by.empty()) out << "no independent xrefs";
        else out << "Ghidra xrefs provide a plausible bounded follow-up";
        out << "\n";
        if (++audited == 10U) break;
    }
    out << "note: ranking is prioritization, not evidence confidence; Ghidra-only records are never auto-promoted.\n";
    return out.str();
}

} // namespace oasis::tools
