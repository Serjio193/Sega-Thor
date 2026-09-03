#include "tools/re_resolution.hpp"

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
    return result + '"';
}

void json_counts(std::ostringstream& out, const std::vector<ResolutionCount>& values) {
    out << '[';
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i) out << ',';
        out << "{\"key\":" << json_string(values[i].key) << ",\"count\":" << values[i].count << '}';
    }
    out << ']';
}

} // namespace

std::string resolution_to_json(const ResolutionReport& report) {
    std::ostringstream out;
    out << "{\"schema\":\"oasis.m68k.re-resolution.v1\",\"target_entry\":" << json_string(hex32(report.target_entry))
        << ",\"metrics\":{\"static_candidate_count\":" << report.static_candidate_count
        << ",\"newly_resolved\":" << report.newly_resolved
        << ",\"still_unresolved\":" << report.still_unresolved
        << ",\"provenance_failures\":" << report.provenance_failures
        << ",\"rom_effective_address_count\":" << report.rom_effective_address_count
        << ",\"ram_effective_address_count\":" << report.ram_effective_address_count
        << ",\"unique_concrete_address_count\":" << report.unique_concrete_address_count
        << ",\"atlas_unresolved_before\":" << report.atlas_unresolved_before
        << ",\"atlas_unresolved_after\":" << report.atlas_unresolved_after << "},\"reason_counts\":";
    json_counts(out, report.reason_counts);
    out << ",\"base_register_counts\":";
    json_counts(out, report.base_register_counts);
    out << ",\"ranking_delta\":[";
    for (std::size_t i = 0; i < report.ranking_delta.size(); ++i) {
        if (i) out << ',';
        const auto& item = report.ranking_delta[i];
        out << "{\"dimension\":" << json_string(item.dimension) << ",\"key\":" << json_string(item.key)
            << ",\"before\":" << item.before << ",\"after\":" << item.after << ",\"delta\":" << item.delta << '}';
    }
    out << "],\"effective_address_ranges\":[";
    for (std::size_t i = 0; i < report.effective_address_ranges.size(); ++i) {
        if (i) out << ',';
        const auto& item = report.effective_address_ranges[i];
        out << "{\"class\":" << json_string(effective_address_class_name(item.address_class))
            << ",\"start\":" << json_string(hex32(item.start)) << ",\"end\":" << json_string(hex32(item.end))
            << ",\"count\":" << item.count << '}';
    }
    out << "],\"items\":[";
    for (std::size_t i = 0; i < report.items.size(); ++i) {
        if (i) out << ',';
        const auto& item = report.items[i];
        out << "{\"instruction_address\":" << json_string(hex32(item.instruction_address))
            << ",\"block_start\":" << json_string(hex32(item.block_start))
            << ",\"base_register\":" << json_string("A" + std::to_string(item.base_register))
            << ",\"displacement\":" << item.displacement
            << ",\"status\":" << json_string(resolution_status_name(item.status))
            << ",\"address_class\":" << json_string(effective_address_class_name(item.address_class))
            << ",\"base_value\":";
        if (item.base_value) out << json_string(hex32(*item.base_value)); else out << "null";
        out << ",\"effective_address\":";
        if (item.effective_address) out << json_string(hex32(*item.effective_address)); else out << "null";
        out << ",\"reason\":" << json_string(item.reason) << ",\"provenance\":[";
        for (std::size_t j = 0; j < item.provenance.size(); ++j) {
            if (j) out << ',';
            const auto& step = item.provenance[j];
            out << "{\"instruction_address\":" << json_string(hex32(step.instruction_address))
                << ",\"block_start\":" << json_string(hex32(step.block_start))
                << ",\"operation\":" << json_string(step.operation)
                << ",\"value\":" << json_string(hex32(step.value)) << '}';
        }
        out << "]}";
    }
    return out.str() + "]}";
}

std::string resolution_to_text(const ResolutionReport& report) {
    std::ostringstream out;
    out << "oasis.m68k.re-resolution.v1\n"
        << "target=" << hex32(report.target_entry) << " candidates=" << report.static_candidate_count
        << " resolved=" << report.newly_resolved << " still_unresolved=" << report.still_unresolved
        << " atlas=" << report.atlas_unresolved_before << " -> " << report.atlas_unresolved_after << "\n";
    out << "reasons:";
    for (const auto& item : report.reason_counts) out << ' ' << item.key << '=' << item.count;
    out << "\nbase_registers:";
    for (const auto& item : report.base_register_counts) out << ' ' << item.key << '=' << item.count;
    out << "\nranges:\n";
    for (const auto& item : report.effective_address_ranges)
        out << "- " << effective_address_class_name(item.address_class) << ' ' << hex32(item.start) << ".." << hex32(item.end)
            << " count=" << item.count << '\n';
    out << "ranking_delta:\n";
    for (const auto& item : report.ranking_delta)
        out << "- " << item.dimension << ':' << item.key << ' ' << item.before << " -> " << item.after
            << " delta=" << item.delta << '\n';
    out << "provenance_examples:\n";
    std::size_t examples = 0;
    for (const auto& item : report.items) {
        if (item.status != ResolutionStatus::resolved || item.provenance.empty() || examples == 5U) continue;
        out << "- " << hex32(item.instruction_address) << " " << effective_address_class_name(item.address_class)
            << " effective=" << hex32(*item.effective_address) << " via";
        for (const auto& step : item.provenance)
            out << ' ' << hex32(step.instruction_address) << ':' << step.operation << '=' << hex32(step.value);
        out << '\n';
        ++examples;
    }
    return out.str();
}

} // namespace oasis::tools
