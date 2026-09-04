#include "tools/re_callee_effect.hpp"

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
        if (character == '\n') result += 'n';
        else if (character == '\r') result += 'r';
        else if (character == '\t') result += 't';
        else result += character;
    }
    return result + '"';
}

void addresses(std::ostringstream& out, const std::vector<std::uint32_t>& values) {
    out << '[';
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index) out << ',';
        out << json_string(hex32(values[index]));
    }
    out << ']';
}

void memory(std::ostringstream& out, const std::vector<CalleeMemoryEvidence>& values) {
    out << '[';
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index) out << ',';
        const auto& item = values[index];
        out << "{\"instruction_address\":" << json_string(hex32(item.instruction_address))
            << ",\"block_start\":" << json_string(hex32(item.block_start))
            << ",\"address\":";
        if (item.address) out << json_string(hex32(*item.address)); else out << "null";
        out << ",\"width_bytes\":" << static_cast<unsigned>(item.width_bytes)
            << ",\"access\":" << json_string(item.access)
            << ",\"classification\":" << json_string(item.classification)
            << ",\"reason\":" << json_string(item.reason) << '}';
    }
    out << ']';
}

void target_rechecks(std::ostringstream& out, const std::vector<CalleeTargetRecheck>& values) {
    out << '[';
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index) out << ',';
        const auto& item = values[index];
        out << "{\"instruction_address\":" << json_string(hex32(item.instruction_address))
            << ",\"prior_reason\":" << json_string(item.prior_reason)
            << ",\"provenance\":" << json_string(item.provenance)
            << ",\"a7_after_call\":" << json_string(item.a7_after_call)
            << ",\"stack_value\":" << json_string(item.stack_value)
            << ",\"effective_address\":";
        if (item.effective_address) out << json_string(hex32(*item.effective_address)); else out << "null";
        out << ",\"address_class\":" << json_string(item.address_class)
            << ",\"status\":" << json_string(item.status) << '}';
    }
    out << ']';
}

void edges(std::ostringstream& out, const std::vector<ControlFlowEdge>& values) {
    out << '[';
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index) out << ',';
        const auto& edge = values[index];
        out << "{\"source\":" << json_string(hex32(edge.source))
            << ",\"target\":" << json_string(hex32(edge.target))
            << ",\"kind\":" << json_string(flow_kind_name(edge.kind)) << '}';
    }
    out << ']';
}

} // namespace

std::string callee_effect_to_json(const CalleeEffectReport& report) {
    std::ostringstream out;
    out << "{\"schema\":\"oasis.m68k.re-callee-effect.v1\""
        << ",\"requested_entry\":" << json_string(hex32(report.requested_entry))
        << ",\"call_site\":" << json_string(hex32(report.call_site))
        << ",\"entry\":" << json_string(hex32(report.entry))
        << ",\"bounded_range\":{\"start\":" << json_string(hex32(report.bounded_start))
        << ",\"end\":" << json_string(hex32(report.bounded_end)) << '}'
        << ",\"boundary\":{\"proven\":" << (report.boundary_proven ? "true" : "false")
        << ",\"status\":" << json_string(report.boundary_status) << '}'
        << ",\"reachable_blocks\":";
    addresses(out, report.reachable_blocks);
    out << ",\"control_flow_edges\":";
    edges(out, report.control_flow_edges);
    out << ",\"return_sites\":";
    addresses(out, report.return_sites);
    out << ",\"register_effects\":[";
    for (std::size_t index = 0; index < report.register_effects.size(); ++index) {
        if (index) out << ',';
        const auto& effect = report.register_effects[index];
        out << "{\"register\":" << json_string("A" + std::to_string(effect.register_index))
            << ",\"effect\":" << json_string(effect.effect) << ",\"known_value\":";
        if (effect.known_value) out << json_string(hex32(*effect.known_value)); else out << "null";
        out << ",\"evidence_instructions\":";
        addresses(out, effect.evidence_instructions);
        out << '}';
    }
    out << "],\"stack_effect\":{\"entry_a7\":" << json_string(report.stack_effect.entry_a7)
        << ",\"bsr_push\":" << json_string(report.stack_effect.bsr_push)
        << ",\"internal_operations\":" << json_string(report.stack_effect.internal_operations)
        << ",\"rts_pop\":" << json_string(report.stack_effect.rts_pop)
        << ",\"net_after_return\":" << json_string(report.stack_effect.net_after_return)
        << ",\"status\":" << json_string(report.stack_effect.status) << '}'
        << ",\"memory_references\":";
    memory(out, report.memory_references);
    out << ",\"unresolved_memory_references\":";
    memory(out, report.unresolved_memory_references);
    out << ",\"calls\":";
    addresses(out, report.direct_callees);
    out << ",\"indirect_flow\":";
    addresses(out, report.indirect_flow);
    out << ",\"unsupported\":";
    addresses(out, report.unsupported_instructions);
    out << ",\"target_rechecks\":";
    target_rechecks(out, report.target_rechecks);
    out << ",\"provenance\":[";
    for (std::size_t index = 0; index < report.provenance.size(); ++index) {
        if (index) out << ',';
        out << json_string(report.provenance[index]);
    }
    return out.str() + "]}";
}

std::string callee_effect_to_text(const CalleeEffectReport& report) {
    std::ostringstream out;
    out << "oasis.m68k.re-callee-effect.v1\n"
        << "requested=" << hex32(report.requested_entry) << " call_site=" << hex32(report.call_site)
        << " callee=" << hex32(report.entry) << " range=" << hex32(report.bounded_start)
        << ".." << hex32(report.bounded_end) << " boundary=" << (report.boundary_proven ? "proven" : "bounded") << '\n'
        << "blocks=" << report.reachable_blocks.size() << " returns=" << report.return_sites.size()
        << " calls=" << report.direct_callees.size() << " memory=" << report.memory_references.size()
        << " unresolved_memory=" << report.unresolved_memory_references.size()
        << " unsupported=" << report.unsupported_instructions.size()
        << " edges=" << report.control_flow_edges.size() << "\nregister_effects:\n";
    for (const auto& effect : report.register_effects) {
        out << "- A" << static_cast<unsigned>(effect.register_index) << ' ' << effect.effect;
        if (effect.known_value) out << " value=" << hex32(*effect.known_value);
        out << " evidence=" << effect.evidence_instructions.size() << '\n';
    }
    out << "stack:\n- entry_a7=" << report.stack_effect.entry_a7
        << " bsr_push=" << report.stack_effect.bsr_push
        << " internal=" << report.stack_effect.internal_operations
        << " rts_pop=" << report.stack_effect.rts_pop
        << " net=" << report.stack_effect.net_after_return
        << " status=" << report.stack_effect.status << "\nreturns:";
    for (const auto address : report.return_sites) out << ' ' << hex32(address);
    out << "\ntarget_rechecks:\n";
    for (const auto& item : report.target_rechecks) {
        out << "- " << hex32(item.instruction_address) << " status=" << item.status
            << " a7=" << item.a7_after_call << " stack=" << item.stack_value << '\n';
    }
    out << "provenance:\n";
    for (const auto& item : report.provenance) out << "- " << item << '\n';
    return out.str();
}

} // namespace oasis::tools
