#include "tools/re_mass_verify.hpp"

#include <algorithm>
#include <iomanip>
#include <iterator>
#include <sstream>

namespace oasis::tools {
namespace {

std::string hex32(std::uint32_t value) {
    std::ostringstream out;
    out << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << value;
    return out.str();
}

std::string quote(const std::string& value) {
    std::string result = "\"";
    for (const char character : value) {
        if (character == '\\' || character == '"') result += '\\';
        if (character == '\n') result += 'n';
        else if (character == '\r') result += 'r';
        else if (character == '\t') result += 't';
        else result += character;
    }
    return result + '"';
}

template <typename T>
void json_addresses(std::ostringstream& out, const std::vector<T>& values) {
    out << '[';
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i) out << ',';
        out << quote(hex32(values[i]));
    }
    out << ']';
}

void json_candidate(std::ostringstream& out, const MassCandidate& item) {
    out << "{\"entry\":" << quote(hex32(item.entry))
        << ",\"previous_classification\":" << quote(candidate_classification_name(item.previous_classification))
        << ",\"complexity\":" << quote(candidate_complexity_name(item.complexity))
        << ",\"decode_ok\":" << (item.decode_ok ? "true" : "false")
        << ",\"first_instruction_supported\":" << (item.first_instruction_supported ? "true" : "false")
        << ",\"direct_bsr_target\":" << (item.direct_bsr_target ? "true" : "false")
        << ",\"direct_jsr_target\":" << (item.direct_jsr_target ? "true" : "false")
        << ",\"known_static_target\":" << (item.known_static_target ? "true" : "false")
        << ",\"vector_target\":" << (item.vector_target ? "true" : "false")
        << ",\"direct_caller_count\":" << item.direct_caller_count
        << ",\"reachable_instruction_count\":" << item.reachable_instruction_count
        << ",\"reachable_block_count\":" << item.reachable_block_count
        << ",\"has_rts\":" << (item.reaches_rts ? "true" : "false")
        << ",\"has_rte\":" << (item.reaches_rte ? "true" : "false")
        << ",\"ends_known_direct_transfer\":" << (item.ends_known_direct_transfer ? "true" : "false")
        << ",\"unresolved_indirect_flow\":" << (item.unresolved_indirect_flow ? "true" : "false")
        << ",\"unsupported_opcode_count\":" << item.unsupported_opcode_count
        << ",\"unsupported_addressing_count\":" << item.unsupported_addressing_count
        << ",\"decode_conflict\":" << (item.decode_conflict ? "true" : "false")
        << ",\"boundary_status\":" << quote(mass_boundary_name(item.boundary_status))
        << ",\"known_data_overlap\":" << (item.known_data_overlap ? "true" : "false")
        << ",\"confirmed_code_overlap\":" << (item.confirmed_code_overlap ? "true" : "false")
        << ",\"other_ghidra_overlap\":" << (item.other_ghidra_overlap ? "true" : "false")
        << ",\"existing_beta_support\":" << (item.existing_beta_support ? "true" : "false")
        << ",\"existing_dynamic_support\":" << (item.existing_dynamic_support ? "true" : "false")
        << ",\"structural_classification\":" << quote(structural_classification_name(item.structural_classification))
        << ",\"failure_reasons\":[";
    for (std::size_t i = 0; i < item.failure_reasons.size(); ++i) {
        if (i) out << ',';
        out << quote(item.failure_reasons[i]);
    }
    out << "]}";
}

std::size_t previous_count(const MassVerificationReport& report, CandidateClassification value) {
    return static_cast<std::size_t>(std::count_if(report.candidates.begin(), report.candidates.end(),
        [=](const auto& item) { return item.previous_classification == value; }));
}

std::size_t structural_count(const MassVerificationReport& report, CandidateClassification previous,
                             StructuralClassification value) {
    return static_cast<std::size_t>(std::count_if(report.candidates.begin(), report.candidates.end(),
        [=](const auto& item) {
            return item.previous_classification == previous && item.structural_classification == value;
        }));
}

void json_structural_counts(std::ostringstream& out, const MassVerificationReport& report,
                            CandidateClassification previous) {
    constexpr StructuralClassification values[] = {
        StructuralClassification::strong_static, StructuralClassification::moderate_static,
        StructuralClassification::weak_static, StructuralClassification::indirect_flow,
        StructuralClassification::unsupported, StructuralClassification::boundary_conflict,
        StructuralClassification::data_conflict, StructuralClassification::decode_failure,
        StructuralClassification::unknown};
    out << '{';
    for (std::size_t i = 0; i < std::size(values); ++i) {
        if (i) out << ',';
        out << quote(structural_classification_name(values[i])) << ':'
            << structural_count(report, previous, values[i]);
    }
    out << '}';
}

} // namespace

std::string mass_verify_to_json(const MassVerificationReport& report) {
    std::ostringstream out;
    out << "{\"schema\":" << quote(report.schema)
        << ",\"candidate_map_schema\":" << quote(report.candidate_map_schema)
        << ",\"ghidra\":{\"schema\":" << quote(report.ghidra_schema)
        << ",\"program\":" << quote(report.ghidra_program) << "}"
        << ",\"total_candidates\":" << report.total_candidates
        << ",\"previous_classification_counts\":{\"CONFIRMED\":"
        << previous_count(report, CandidateClassification::confirmed)
        << ",\"STATIC_SUPPORTED\":" << previous_count(report, CandidateClassification::static_supported)
        << ",\"DYNAMIC_OBSERVED\":" << previous_count(report, CandidateClassification::dynamic_observed)
        << ",\"GHIDRA_ONLY\":" << previous_count(report, CandidateClassification::ghidra_only)
        << ",\"CONFLICT\":" << previous_count(report, CandidateClassification::conflict) << '}'
        << ",\"ghidra_only_structural_counts\":";
    json_structural_counts(out, report, CandidateClassification::ghidra_only);
    out << ",\"static_supported_structural_counts\":";
    json_structural_counts(out, report, CandidateClassification::static_supported);
    out
        << ",\"candidates\":[";
    for (std::size_t i = 0; i < report.candidates.size(); ++i) {
        if (i) out << ',';
        json_candidate(out, report.candidates[i]);
    }
    out << "],\"failure_clusters\":[";
    for (std::size_t i = 0; i < report.failure_clusters.size(); ++i) {
        if (i) out << ',';
        const auto& cluster = report.failure_clusters[i];
        out << "{\"reason\":" << quote(cluster.reason) << ",\"count\":" << cluster.count
            << ",\"percentage\":" << std::fixed << std::setprecision(2)
            << (report.total_candidates ? 100.0 * cluster.count / report.total_candidates : 0.0)
            << ",\"sample_addresses\":";
        json_addresses(out, cluster.sample_addresses);
        out << ",\"systemic_fix_candidate\":" << (cluster.systemic_fix_candidate ? "true" : "false") << '}';
    }
    out << "],\"leaf\":{\"total\":" << report.leaf.total
        << ",\"clean\":" << report.leaf.clean << ",\"unsupported\":" << report.leaf.unsupported
        << ",\"indirect\":" << report.leaf.indirect << ",\"boundary_conflict\":" << report.leaf.boundary_conflict
        << ",\"terminal_failure\":" << report.leaf.terminal_failure << ",\"other\":" << report.leaf.other << "}"
        << ",\"control_set\":[";
    for (std::size_t i = 0; i < report.control_set.size(); ++i) {
        if (i) out << ',';
        const auto& item = report.control_set[i];
        out << "{\"entry\":" << quote(hex32(item.entry)) << ",\"present\":" << (item.present ? "true" : "false")
            << ",\"heuristic_pass\":" << (item.heuristic_pass ? "true" : "false")
            << ",\"heuristic_miss\":" << (item.heuristic_miss ? "true" : "false")
            << ",\"structural_classification\":" << quote(structural_classification_name(item.structural_classification)) << '}';
    }
    out << "],\"top_systemic_fixes\":[";
    for (std::size_t i = 0; i < report.top_systemic_fixes.size(); ++i) {
        if (i) out << ',';
        const auto& fix = report.top_systemic_fixes[i];
        out << "{\"blocker_class\":" << quote(fix.blocker_class) << ",\"affected_count\":" << fix.affected_count
            << ",\"estimated_effort\":" << quote(fix.effort) << ",\"risk\":" << quote(fix.risk)
            << ",\"expected_gain\":" << quote(fix.expected_gain) << '}';
    }
    out << "]}";
    return out.str();
}

std::string mass_verify_to_text(const MassVerificationReport& report) {
    std::ostringstream out;
    out << "oasis.m68k.re-mass-verify.v1\n"
        << "candidate_map_schema=" << report.candidate_map_schema << " ghidra=" << report.ghidra_program << "\n"
        << "total_candidates=" << report.total_candidates << "\n"
        << "previous_classification_counts: CONFIRMED=" << previous_count(report, CandidateClassification::confirmed)
        << " STATIC_SUPPORTED=" << previous_count(report, CandidateClassification::static_supported)
        << " DYNAMIC_OBSERVED=" << previous_count(report, CandidateClassification::dynamic_observed)
        << " GHIDRA_ONLY=" << previous_count(report, CandidateClassification::ghidra_only)
        << " CONFLICT=" << previous_count(report, CandidateClassification::conflict) << "\n"
        << "ghidra_only_structural_counts:";
    for (const auto value : {StructuralClassification::strong_static, StructuralClassification::moderate_static,
                             StructuralClassification::weak_static, StructuralClassification::indirect_flow,
                             StructuralClassification::unsupported, StructuralClassification::boundary_conflict,
                             StructuralClassification::data_conflict, StructuralClassification::decode_failure,
                             StructuralClassification::unknown})
        out << ' ' << structural_classification_name(value) << '='
            << structural_count(report, CandidateClassification::ghidra_only, value);
    out << "\nstatic_supported_structural_counts:";
    for (const auto value : {StructuralClassification::strong_static, StructuralClassification::moderate_static,
                             StructuralClassification::weak_static, StructuralClassification::indirect_flow,
                             StructuralClassification::unsupported, StructuralClassification::boundary_conflict,
                             StructuralClassification::data_conflict, StructuralClassification::decode_failure,
                             StructuralClassification::unknown})
        out << ' ' << structural_classification_name(value) << '='
            << structural_count(report, CandidateClassification::static_supported, value);
    out << "\n"
        << "leaf total=" << report.leaf.total << " clean=" << report.leaf.clean
        << " unsupported=" << report.leaf.unsupported << " indirect=" << report.leaf.indirect
        << " boundary_conflict=" << report.leaf.boundary_conflict << " terminal_failure=" << report.leaf.terminal_failure
        << " other=" << report.leaf.other << "\n"
        << "failure_clusters:\n";
    for (const auto& cluster : report.failure_clusters) {
        out << "- " << cluster.reason << " count=" << cluster.count << " percentage=" << std::fixed << std::setprecision(2)
            << (report.total_candidates ? 100.0 * cluster.count / report.total_candidates : 0.0) << " samples=";
        for (std::size_t i = 0; i < cluster.sample_addresses.size(); ++i) {
            if (i) out << ',';
            out << hex32(cluster.sample_addresses[i]);
        }
        out << " systemic_fix=" << (cluster.systemic_fix_candidate ? "yes" : "no") << '\n';
    }
    out << "control_set:\n";
    for (const auto& item : report.control_set)
        out << "- " << hex32(item.entry) << " present=" << (item.present ? "yes" : "no")
            << " heuristic=" << (item.heuristic_pass ? "PASS" : "HEURISTIC_MISS_ON_CONFIRMED")
            << " class=" << structural_classification_name(item.structural_classification) << '\n';
    out << "top_systemic_fixes:\n";
    for (std::size_t i = 0; i < report.top_systemic_fixes.size(); ++i) {
        const auto& fix = report.top_systemic_fixes[i];
        out << i + 1 << ". " << fix.blocker_class << " affected=" << fix.affected_count
            << " effort=" << fix.effort << " risk=" << fix.risk << " expected_gain=" << fix.expected_gain << '\n';
    }
    out << "candidate_rows:\n";
    for (const auto& item : report.candidates) {
        out << hex32(item.entry) << " previous=" << candidate_classification_name(item.previous_classification)
            << " class=" << structural_classification_name(item.structural_classification)
            << " decode=" << (item.decode_ok ? "ok" : "fail")
            << " instructions=" << item.reachable_instruction_count << " blocks=" << item.reachable_block_count
            << " boundary=" << mass_boundary_name(item.boundary_status) << " failures=";
        for (std::size_t i = 0; i < item.failure_reasons.size(); ++i) {
            if (i) out << ',';
            out << item.failure_reasons[i];
        }
        out << '\n';
    }
    return out.str();
}

} // namespace oasis::tools
