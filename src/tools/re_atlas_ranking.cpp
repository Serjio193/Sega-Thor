#include "tools/re_atlas_ranking.hpp"

#include <algorithm>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <utility>

namespace oasis::tools {
namespace {

struct Accumulator {
    AtlasRankingGroup group;
    std::set<std::uint32_t> instructions;
    std::set<std::uint32_t> functions;
};

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

std::string register_name(const AtlasUnresolvedReference& item) {
    if (item.mode == 7U && item.register_index == 3U) return "PC";
    return "A" + std::to_string(item.register_index);
}

} // namespace

AtlasRankingReport rank_atlas_unresolved(const AtlasReport& atlas) {
    AtlasRankingReport report;
    std::map<std::pair<std::string, std::string>, Accumulator> groups;
    const auto add = [&](const std::string& dimension, const std::string& key,
                         std::uint32_t instruction, std::uint32_t function,
                         bool potentially_resolvable) {
        auto& item = groups[{dimension, key}];
        item.group.dimension = dimension;
        item.group.key = key;
        ++item.group.frequency;
        if (potentially_resolvable) ++item.group.potentially_resolvable_refs;
        item.instructions.insert(instruction);
        item.functions.insert(function);
    };

    for (const auto& entry : atlas.entries) {
        for (const auto& unresolved : entry.unresolved_references) {
            ++report.atlas_unresolved_reference_count;
            add("addressing_mode", unresolved.addressing_mode, unresolved.instruction_address,
                entry.start, true);
            add("register", register_name(unresolved), unresolved.instruction_address, entry.start, true);
            add("instruction_family", unresolved.instruction_family, unresolved.instruction_address,
                entry.start, true);
            add("containing_function", hex32(entry.start), unresolved.instruction_address,
                entry.start, true);
            if (unresolved.dynamic_resolvable_candidate) {
                ++report.dynamic_resolvable_candidate_count;
                add("dynamic_resolvable_candidate", "bounded_A6A4_scenario",
                    unresolved.instruction_address, entry.start, true);
            }
            if (unresolved.constant_propagation_candidate) {
                ++report.constant_propagation_candidate_count;
                add("likely_constant_propagation_candidate", "instruction_has_immediate",
                    unresolved.instruction_address, entry.start, true);
            }
        }
        for (const auto& unsupported : entry.unsupported_evidence) {
            ++report.unsupported_decoder_item_count;
            auto& item = groups[{"unsupported_decoder_dependency", unsupported.kind + ":" + unsupported.reason}];
            item.group.dimension = "unsupported_decoder_dependency";
            item.group.key = unsupported.kind + ":" + unsupported.reason;
            ++item.group.unsupported_items;
            item.instructions.insert(unsupported.instruction_address);
            item.functions.insert(entry.start);
        }
    }
    for (auto& [key, item] : groups) {
        item.group.unique_instructions = item.instructions.size();
        item.group.unique_functions = item.functions.size();
        item.group.evidence_basis = item.group.dimension == "unsupported_decoder_dependency"
                                        ? "unsupported decoder evidence; potential ref count is not estimated"
                                        : "structural candidate count; not proof of resolution";
        report.groups.push_back(std::move(item.group));
    }
    std::sort(report.groups.begin(), report.groups.end(), [](const auto& left, const auto& right) {
        return std::tie(left.potentially_resolvable_refs, left.frequency, left.dimension, left.key) >
               std::tie(right.potentially_resolvable_refs, right.frequency, right.dimension, right.key);
    });
    return report;
}

std::string ranking_to_json(const AtlasRankingReport& report) {
    std::ostringstream out;
    out << "{\"schema\":\"oasis.m68k.re-ranking.v1\",\"atlas_unresolved_reference_count\":"
        << report.atlas_unresolved_reference_count
        << ",\"dynamic_resolvable_candidate_count\":" << report.dynamic_resolvable_candidate_count
        << ",\"constant_propagation_candidate_count\":" << report.constant_propagation_candidate_count
        << ",\"unsupported_decoder_item_count\":" << report.unsupported_decoder_item_count
        << ",\"groups\":[";
    for (std::size_t i = 0; i < report.groups.size(); ++i) {
        if (i) out << ',';
        const auto& group = report.groups[i];
        out << "{\"dimension\":" << json_string(group.dimension)
            << ",\"key\":" << json_string(group.key)
            << ",\"frequency\":" << group.frequency
            << ",\"potentially_resolvable_refs\":" << group.potentially_resolvable_refs
            << ",\"unsupported_items\":" << group.unsupported_items
            << ",\"unique_instructions\":" << group.unique_instructions
            << ",\"unique_functions\":" << group.unique_functions
            << ",\"evidence_basis\":" << json_string(group.evidence_basis) << '}';
    }
    out << "]}";
    return out.str();
}

std::string ranking_to_text(const AtlasRankingReport& report) {
    std::ostringstream out;
    out << "oasis.m68k.re-ranking.v1\n"
        << "atlas_unresolved_reference_count: " << report.atlas_unresolved_reference_count << "\n"
        << "dynamic_resolvable_candidate_count: " << report.dynamic_resolvable_candidate_count << "\n"
        << "constant_propagation_candidate_count: " << report.constant_propagation_candidate_count << "\n"
        << "unsupported_decoder_item_count: " << report.unsupported_decoder_item_count << "\n"
        << "priority_opportunities:\n";
    for (const auto& group : report.groups) {
        out << "  - If support " << group.dimension << ':' << group.key
            << ": potentially resolve " << group.potentially_resolvable_refs << " refs"
            << " (frequency " << group.frequency << ", instructions " << group.unique_instructions
            << ", functions " << group.unique_functions;
        if (group.unsupported_items) out << ", unsupported_items " << group.unsupported_items;
        out << ")\n";
    }
    out << "note: dimensions overlap; counts are not additive and are structural candidates, not semantic resolution.\n";
    return out.str();
}

} // namespace oasis::tools
