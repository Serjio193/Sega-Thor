#include "tools/re_caller_stack.hpp"

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

void value(std::ostringstream& out, const std::optional<CallerStackValue>& item) {
    if (!item) { out << "null"; return; }
    out << "{\"expression\":" << json_string(item->expression)
        << ",\"kind\":" << json_string(item->kind)
        << ",\"source_instruction\":" << json_string(hex32(item->source_instruction)) << '}';
}

void event(std::ostringstream& out, const CallerStackEvent& item) {
    out << "{\"instruction_address\":" << json_string(hex32(item.instruction_address))
        << ",\"block_start\":" << json_string(hex32(item.block_start))
        << ",\"event\":" << json_string(item.event)
        << ",\"sp_before\":" << json_string(item.sp_before)
        << ",\"sp_after\":" << json_string(item.sp_after)
        << ",\"value\":" << json_string(item.value) << '}';
}

void events(std::ostringstream& out, const std::vector<CallerStackEvent>& items) {
    out << '[';
    for (std::size_t index = 0; index < items.size(); ++index) {
        if (index) out << ',';
        event(out, items[index]);
    }
    out << ']';
}

void path(std::ostringstream& out, const CallerStackPath& item) {
    out << "{\"block_starts\":";
    addresses(out, item.block_starts);
    out << ",\"events\":";
    events(out, item.events);
    out << ",\"pre_call_sp_offset\":";
    if (item.pre_call_sp_offset) out << *item.pre_call_sp_offset; else out << "null";
    out << ",\"value_at_pre_call_sp\":";
    value(out, item.value_at_pre_call_sp);
    out << ",\"status\":" << json_string(item.status) << ",\"blockers\":[";
    for (std::size_t index = 0; index < item.blockers.size(); ++index) {
        if (index) out << ',';
        out << json_string(item.blockers[index]);
    }
    out << "]}";
}

void target_results(std::ostringstream& out, const std::vector<CallerStackTargetResult>& items) {
    out << '[';
    for (std::size_t index = 0; index < items.size(); ++index) {
        if (index) out << ',';
        const auto& item = items[index];
        out << "{\"instruction_address\":" << json_string(hex32(item.instruction_address))
            << ",\"prior_reason\":" << json_string(item.prior_reason)
            << ",\"symbolic_a7_before\":" << json_string(item.symbolic_a7_before)
            << ",\"stack_value\":" << json_string(item.stack_value)
            << ",\"value_kind\":" << json_string(item.value_kind)
            << ",\"effective_address\":";
        if (item.effective_address) out << json_string(hex32(*item.effective_address)); else out << "null";
        out << ",\"address_class\":" << json_string(item.address_class)
            << ",\"provenance\":" << json_string(item.provenance)
            << ",\"status\":" << json_string(item.status) << '}';
    }
    out << ']';
}

} // namespace

std::string caller_stack_to_json(const CallerStackReport& report) {
    std::ostringstream out;
    out << "{\"schema\":\"oasis.m68k.re-caller-stack.v1\""
        << ",\"entry\":" << json_string(hex32(report.entry))
        << ",\"call_site\":" << json_string(hex32(report.call_site))
        << ",\"callee\":" << json_string(hex32(report.callee))
        << ",\"window\":{\"start\":" << json_string(hex32(report.window_start))
        << ",\"end\":" << json_string(hex32(report.window_end)) << '}'
        << ",\"symbolic_entry_sp\":" << json_string(report.symbolic_entry_sp)
        << ",\"symbolic_pre_call_sp\":" << json_string(report.symbolic_pre_call_sp)
        << ",\"containing_blocks\":";
    addresses(out, report.containing_blocks);
    out << ",\"predecessor_blocks\":";
    addresses(out, report.predecessor_blocks);
    out << ",\"stack_events\":";
    events(out, report.stack_events);
    out << ",\"cfg_paths\":[";
    for (std::size_t index = 0; index < report.cfg_paths.size(); ++index) {
        if (index) out << ',';
        path(out, report.cfg_paths[index]);
    }
    out << "],\"merge_status\":" << json_string(report.merge_status)
        << ",\"value_at_pre_call_sp\":";
    value(out, report.value_at_pre_call_sp);
    out << ",\"value_kind\":" << json_string(report.value_kind) << ",\"provenance\":[";
    for (std::size_t index = 0; index < report.provenance.size(); ++index) {
        if (index) out << ',';
        out << json_string(report.provenance[index]);
    }
    out << "],\"blockers\":[";
    for (std::size_t index = 0; index < report.blockers.size(); ++index) {
        if (index) out << ',';
        out << json_string(report.blockers[index]);
    }
    out << "],\"target_results\":";
    target_results(out, report.target_results);
    out << ",\"metrics\":{\"relevant_paths\":" << report.relevant_path_count
        << ",\"stack_events\":" << report.stack_event_count
        << ",\"prior_calls_crossed\":" << report.prior_calls_crossed
        << ",\"known_call_effects\":" << report.known_call_effects
        << ",\"unknown_call_effects\":" << report.unknown_call_effects
        << ",\"stack_merge_conflicts\":" << report.stack_merge_conflicts
        << ",\"reachable_unresolved_before\":" << report.reachable_unresolved_before
        << ",\"reachable_unresolved_after\":" << report.reachable_unresolved_after
        << ",\"speculative_resolutions\":" << report.speculative_resolutions << "}}";
    return out.str();
}

std::string caller_stack_to_text(const CallerStackReport& report) {
    std::ostringstream out;
    out << "oasis.m68k.re-caller-stack.v1\n"
        << "entry=" << hex32(report.entry) << " call_site=" << hex32(report.call_site)
        << " callee=" << hex32(report.callee) << " window=" << hex32(report.window_start)
        << ".." << hex32(report.window_end) << '\n'
        << "symbolic_sp: entry=" << report.symbolic_entry_sp << " pre_call=" << report.symbolic_pre_call_sp
        << " value_kind=" << report.value_kind << " merge=" << report.merge_status << '\n'
        << "paths=" << report.relevant_path_count << " events=" << report.stack_event_count
        << " calls=" << report.prior_calls_crossed << " known_calls=" << report.known_call_effects
        << " unknown_calls=" << report.unknown_call_effects << " conflicts=" << report.stack_merge_conflicts << '\n';
    out << "value_at_pre_call_sp=";
    if (report.value_at_pre_call_sp) out << report.value_at_pre_call_sp->expression;
    else out << "UNKNOWN";
    out << "\ntargets:\n";
    for (const auto& item : report.target_results)
        out << "- " << hex32(item.instruction_address) << " status=" << item.status
            << " stack=" << item.stack_value << " effective="
            << (item.effective_address ? hex32(*item.effective_address) : "UNKNOWN") << '\n';
    out << "blockers:\n";
    for (const auto& item : report.blockers) out << "- " << item << '\n';
    return out.str();
}

} // namespace oasis::tools
