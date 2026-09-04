#include "tools/re_reachable_closure.hpp"

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
    out << std::uppercase << std::hex << std::setfill('0');
    for (const auto byte : bytes) out << std::setw(2) << static_cast<unsigned>(byte);
    return out.str();
}

std::string json_string(const std::string& value) {
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

void json_addresses(std::ostringstream& out, const std::vector<std::uint32_t>& values) {
    out << '[';
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i) out << ',';
        out << json_string(hex32(values[i]));
    }
    out << ']';
}

void json_proof(std::ostringstream& out, const ResolutionProofStep& step) {
    out << "{\"instruction_address\":" << json_string(hex32(step.instruction_address))
        << ",\"block_start\":" << json_string(hex32(step.block_start))
        << ",\"operation\":" << json_string(step.operation)
        << ",\"value\":" << json_string(hex32(step.value)) << '}';
}

void json_item(std::ostringstream& out, const ReachableClosureItem& item) {
    out << "{\"instruction_address\":" << json_string(hex32(item.instruction_address))
        << ",\"block_start\":" << json_string(hex32(item.block_start))
        << ",\"opcode\":" << json_string(hex32(item.opcode))
        << ",\"bytes\":" << json_string(hex_bytes(item.bytes))
        << ",\"mnemonic\":" << json_string(item.mnemonic) << ",\"addressing_modes\":[";
    for (std::size_t i = 0; i < item.addressing_modes.size(); ++i) {
        if (i) out << ',';
        out << json_string(item.addressing_modes[i]);
    }
    out << "],\"operand\":" << json_string(item.operand)
        << ",\"base_register\":" << json_string("A" + std::to_string(item.base_register))
        << ",\"displacement\":" << item.displacement << ",\"cfg_predecessors\":";
    json_addresses(out, item.cfg_predecessors);
    out << ",\"initial_status\":" << json_string(resolution_status_name(item.initial_status))
        << ",\"current_unresolved_reason\":" << json_string(item.current_unresolved_reason)
        << ",\"prior_closure_reason\":" << json_string(item.prior_closure_reason)
        << ",\"closure_reason\":" << json_string(closure_reason_name(item.reason))
        << ",\"last_known_definitions\":[";
    for (std::size_t i = 0; i < item.last_known_definitions.size(); ++i) {
        if (i) out << ',';
        const auto& definition = item.last_known_definitions[i];
        out << "{\"instruction_address\":" << json_string(hex32(definition.instruction_address))
            << ",\"block_start\":" << json_string(hex32(definition.block_start))
            << ",\"operation\":" << json_string(definition.operation) << ",\"value\":";
        if (definition.value) out << json_string(hex32(*definition.value)); else out << "null";
        out << ",\"supported\":" << (definition.supported ? "true" : "false") << '}';
    }
    out << "],\"nearest_proven_register_state\":";
    if (item.nearest_proven_register_state) json_proof(out, *item.nearest_proven_register_state); else out << "null";
    out << ",\"effective_address\":";
    if (item.effective_address) out << json_string(hex32(*item.effective_address)); else out << "null";
    out << ",\"address_class\":" << json_string(effective_address_class_name(item.address_class))
        << ",\"provenance\":[";
    for (std::size_t i = 0; i < item.provenance.size(); ++i) {
        if (i) out << ',';
        json_proof(out, item.provenance[i]);
    }
    out << "],\"evidence\":" << json_string(item.evidence)
        << ",\"confidence\":" << json_string(item.confidence)
        << ",\"stack_status\":" << json_string(item.stack_status)
        << ",\"a7_before\":";
    if (item.a7_before) out << json_string(hex32(*item.a7_before)); else out << "null";
    out << ",\"a7_increment_bytes\":";
    if (item.a7_increment_bytes) out << *item.a7_increment_bytes; else out << "null";
    out << ",\"stack_provenance\":[";
    for (std::size_t i = 0; i < item.stack_provenance.size(); ++i) {
        if (i) out << ',';
        const auto& definition = item.stack_provenance[i];
        out << "{\"instruction_address\":" << json_string(hex32(definition.instruction_address))
            << ",\"block_start\":" << json_string(hex32(definition.block_start))
            << ",\"operation\":" << json_string(definition.operation) << ",\"value\":";
        if (definition.value) out << json_string(hex32(*definition.value)); else out << "null";
        out << ",\"supported\":" << (definition.supported ? "true" : "false") << '}';
    }
    out << "]" << '}';
}

} // namespace

std::string reachable_closure_to_json(const ReachableClosureReport& report) {
    std::ostringstream out;
    out << "{\"schema\":\"oasis.m68k.re-reachable-closure.v1\",\"target_entry\":"
        << json_string(hex32(report.target_entry)) << ",\"window\":{\"start\":"
        << json_string(hex32(report.window_start)) << ",\"end\":" << json_string(hex32(report.window_end))
        << "},\"metrics\":{\"exact_reachable_unresolved_count\":" << report.exact_reachable_unresolved_count
        << ",\"raw_static_unresolved\":" << report.raw_static_unresolved
        << ",\"raw_displacement_backlog\":" << report.raw_displacement_backlog
        << ",\"reachable_unresolved_before\":" << report.reachable_unresolved_before
        << ",\"newly_resolved\":" << report.newly_resolved
        << ",\"reachable_unresolved_after\":" << report.reachable_unresolved_after
        << ",\"nonreachable_unresolved\":" << report.nonreachable_unresolved
        << ",\"speculative_resolutions\":" << report.speculative_resolutions
        << ",\"provenance_failures\":" << report.provenance_failures
        << ",\"ram_effective_address_count\":" << report.ram_effective_address_count
        << ",\"rom_effective_address_count\":" << report.rom_effective_address_count
        << ",\"atlas_unresolved_before\":" << report.atlas_unresolved_before
        << ",\"atlas_unresolved_after\":" << report.atlas_unresolved_after
        << ",\"ranking_displacement_before\":" << report.ranking_displacement_before
        << ",\"ranking_displacement_after\":" << report.ranking_displacement_after << "},\"dynamic_scenario\":"
        << json_string(report.dynamic_scenario) << ",\"transfer_rule\":"
        << json_string(report.transfer_rule) << ",\"reason_counts\":[";
    for (std::size_t i = 0; i < report.reason_counts.size(); ++i) {
        if (i) out << ',';
        out << "{\"reason\":" << json_string(report.reason_counts[i].key)
            << ",\"count\":" << report.reason_counts[i].count << '}';
    }
    out << "],\"items\":[";
    for (std::size_t i = 0; i < report.items.size(); ++i) {
        if (i) out << ',';
        json_item(out, report.items[i]);
    }
    return out.str() + "]}";
}

std::string reachable_closure_to_text(const ReachableClosureReport& report) {
    std::ostringstream out;
    out << "oasis.m68k.re-reachable-closure.v1\n"
        << "target=" << hex32(report.target_entry) << " window=" << hex32(report.window_start) << ".." << hex32(report.window_end)
        << " exact=" << report.exact_reachable_unresolved_count << " raw=" << report.raw_static_unresolved
        << " displacement=" << report.raw_displacement_backlog << " reachable=" << report.reachable_unresolved_before
        << " -> " << report.reachable_unresolved_after << " newly_resolved=" << report.newly_resolved
        << " nonreachable=" << report.nonreachable_unresolved << " speculative=" << report.speculative_resolutions << '\n'
        << "atlas=" << report.atlas_unresolved_before << " -> " << report.atlas_unresolved_after
        << " displacement_rank=" << report.ranking_displacement_before << " -> " << report.ranking_displacement_after
        << " ram=" << report.ram_effective_address_count << " rom=" << report.rom_effective_address_count
        << " provenance_failures=" << report.provenance_failures << "\ntransfer_rule=" << report.transfer_rule << '\n'
        << "dynamic_scenario=" << report.dynamic_scenario << "\nreasons:";
    for (const auto& count : report.reason_counts) out << ' ' << count.key << '=' << count.count;
    out << "\nitems:\n";
    for (const auto& item : report.items) {
        out << "- " << hex32(item.instruction_address) << " block=" << hex32(item.block_start)
            << " opcode=" << hex32(item.opcode) << " " << item.operand
            << " displacement=" << item.displacement << " initial=" << resolution_status_name(item.initial_status)
            << " prior=" << item.prior_closure_reason << " current=\"" << item.current_unresolved_reason << "\" closure=" << closure_reason_name(item.reason)
            << " confidence=" << item.confidence << " predecessors=";
        for (std::size_t i = 0; i < item.cfg_predecessors.size(); ++i) {
            if (i) out << ',';
            out << hex32(item.cfg_predecessors[i]);
        }
        out << " defs=" << item.last_known_definitions.size() << " stack=" << item.stack_status;
        if (item.a7_before) out << " a7_before=" << hex32(*item.a7_before);
        if (item.a7_increment_bytes) out << " a7_increment=" << *item.a7_increment_bytes;
        if (item.effective_address) out << " effective=" << hex32(*item.effective_address) << ' ' << effective_address_class_name(item.address_class);
        out << '\n';
    }
    return out.str();
}

} // namespace oasis::tools
