#include "tools/re_trace.hpp"

#include <iomanip>
#include <sstream>

namespace oasis::tools {
namespace {

std::string hex_value(std::uint32_t value, unsigned width = 8U) {
    std::ostringstream output;
    output << "0x" << std::hex << std::setfill('0') << std::setw(width) << value;
    return output.str();
}

std::string json_escape(const std::string& value) {
    std::string escaped;
    for (const auto character : value) {
        if (character == '"' || character == '\\') escaped += '\\';
        escaped += character;
    }
    return escaped;
}

void append_snapshot(std::ostringstream& output, const TraceRegisterSnapshot& registers) {
    output << "{\"a0\":";
    if (registers.a0) output << '"' << hex_value(*registers.a0) << '"';
    else output << "null";
    output << ",\"a6\":";
    if (registers.a6) output << '"' << hex_value(*registers.a6) << '"';
    else output << "null";
    output << '}';
}

template <typename Item, typename Writer>
void append_array(std::ostringstream& output, const std::vector<Item>& items, Writer writer) {
    output << '[';
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (i != 0U) output << ',';
        writer(output, items[i]);
    }
    output << ']';
}

} // namespace

std::string trace_to_json(const TraceReport& report) {
    std::ostringstream output;
    output << "{\"schema\":\"oasis.m68k.re-trace.v1\",\"function_entry\":\""
           << hex_value(report.function_entry) << "\",\"start_pc\":\""
           << hex_value(report.start_pc) << "\",\"instruction_budget\":"
           << report.instruction_budget << ",\"stop_reason\":\""
           << json_escape(report.stop_reason) << "\",\"executed_instructions\":";
    append_array(output, report.executed_instructions, [](auto& out, const auto& item) {
        out << "{\"step\":" << item.step << ",\"pc\":\"" << hex_value(item.address)
            << "\",\"opcode\":\"" << hex_value(item.opcode, 4U)
            << "\",\"block\":\"" << hex_value(item.block_start) << '"';
        if (item.registers) {
            out << ",\"registers\":";
            append_snapshot(out, *item.registers);
        }
        out << '}';
    });
    output << ",\"executed_basic_blocks\":";
    append_array(output, report.executed_basic_blocks, [](auto& out, const auto& item) {
        out << '"' << hex_value(item) << '"';
    });
    output << ",\"branch_outcomes\":";
    append_array(output, report.branches, [](auto& out, const auto& item) {
        out << "{\"instruction\":\"" << hex_value(item.instruction_address)
            << "\",\"block\":\"" << hex_value(item.block_start)
            << "\",\"target\":\"" << hex_value(item.target)
            << "\",\"taken\":" << (item.taken ? "true" : "false") << '}';
    });
    output << ",\"calls\":";
    append_array(output, report.calls, [](auto& out, const auto& item) {
        out << "{\"instruction\":\"" << hex_value(item.instruction_address)
            << "\",\"block\":\"" << hex_value(item.block_start)
            << "\",\"target\":\"" << hex_value(item.target)
            << "\",\"kind\":\"" << flow_kind_name(item.kind) << "\"}";
    });
    output << ",\"returns\":";
    append_array(output, report.returns, [](auto& out, const auto& item) {
        out << "{\"instruction\":\"" << hex_value(item.instruction_address)
            << "\",\"block\":\"" << hex_value(item.block_start) << "\"}";
    });
    auto append_memory = [](auto& out, const auto& item) {
        out << "{\"instruction\":\"" << hex_value(item.instruction_address)
            << "\",\"block\":\"" << hex_value(item.block_start)
            << "\",\"address\":\"" << hex_value(item.address)
            << "\",\"width_bytes\":" << static_cast<unsigned>(item.width_bytes)
            << ",\"access\":\"" << memory_access_name(item.access)
            << "\",\"value\":\"" << hex_value(item.value) << '"';
        if (item.registers) {
            out << ",\"registers\":";
            append_snapshot(out, *item.registers);
        }
        out << '}';
    };
    output << ",\"memory_reads\":";
    append_array(output, report.memory_reads, append_memory);
    output << ",\"memory_writes\":";
    append_array(output, report.memory_writes, append_memory);
    output << ",\"indirect_targets\":";
    append_array(output, report.indirect_targets, [](auto& out, const auto& item) {
        out << "{\"instruction\":\"" << hex_value(item.instruction_address)
            << "\",\"block\":\"" << hex_value(item.block_start)
            << "\",\"target\":\"" << hex_value(item.target) << '"';
        if (item.registers) {
            out << ",\"registers\":";
            append_snapshot(out, *item.registers);
        }
        out << '}';
    });
    output << ",\"static_confirmed\":";
    append_array(output, report.static_confirmed, [](auto& out, const auto& item) {
        out << "{\"instruction\":\"" << hex_value(item.instruction_address)
            << "\",\"block\":\"" << hex_value(item.block_start)
            << "\",\"kind\":\"" << json_escape(item.kind) << '"';
        if (item.address) out << ",\"address\":\"" << hex_value(*item.address) << '"';
        else out << ",\"address\":null";
        out << ",\"reason\":\"" << json_escape(item.reason) << "\"}";
    });
    output << ",\"dynamic_observed\":";
    append_array(output, report.dynamic_observed, [](auto& out, const auto& item) {
        out << "{\"instruction\":\"" << hex_value(item.instruction_address)
            << "\",\"block\":\"" << hex_value(item.block_start)
            << "\",\"kind\":\"" << json_escape(item.kind)
            << "\",\"observed_value\":\"" << hex_value(item.observed_value)
            << "\",\"width_bytes\":" << static_cast<unsigned>(item.width_bytes) << '}';
    });
    output << ",\"newly_resolved\":";
    append_array(output, report.newly_resolved, [](auto& out, const auto& item) {
        out << "{\"instruction\":\"" << hex_value(item.instruction_address)
            << "\",\"block\":\"" << hex_value(item.block_start)
            << "\",\"static_kind\":\"" << json_escape(item.static_kind)
            << "\",\"observed_value\":\"" << hex_value(item.observed_value)
            << "\",\"width_bytes\":" << static_cast<unsigned>(item.width_bytes) << '}';
    });
    output << ",\"still_unresolved\":";
    append_array(output, report.still_unresolved, [](auto& out, const auto& item) {
        out << "{\"instruction\":\"" << hex_value(item.instruction_address)
            << "\",\"block\":\"" << hex_value(item.block_start)
            << "\",\"kind\":\"" << json_escape(item.kind)
            << "\",\"reason\":\"" << json_escape(item.reason) << "\"}";
    });
    output << ",\"static_unsupported_opcodes\":[";
    for (std::size_t i = 0; i < report.static_slice.unsupported_instruction_addresses.size(); ++i) {
        if (i != 0U) output << ',';
        output << '"' << hex_value(report.static_slice.unsupported_instruction_addresses[i]) << '"';
    }
    output << "]}";
    return output.str();
}

std::string trace_to_text(const TraceReport& report) {
    std::ostringstream output;
    output << "M68K bounded dynamic RE trace\nfunction=" << hex_value(report.function_entry)
           << " start=" << hex_value(report.start_pc) << " stop=" << report.stop_reason
           << " budget=" << report.instruction_budget << '\n'
           << "executed_instructions=" << report.executed_instructions.size()
           << " executed_basic_blocks=" << report.executed_basic_blocks.size()
           << " branches=" << report.branches.size() << " calls=" << report.calls.size()
           << " returns=" << report.returns.size() << " memory_reads=" << report.memory_reads.size()
           << " memory_writes=" << report.memory_writes.size()
           << " indirect_targets=" << report.indirect_targets.size() << '\n'
           << "static_confirmed=" << report.static_confirmed.size() << '\n'
           << "dynamic_observed=" << report.dynamic_observed.size()
           << "\nnewly_resolved=" << report.newly_resolved.size()
           << "\nstill_unresolved=" << report.still_unresolved.size() << '\n';
    output << "\nExecuted PC trace:\n";
    for (const auto& item : report.executed_instructions) {
        output << "  " << item.step << " " << hex_value(item.address)
               << " block=" << hex_value(item.block_start) << " opcode="
               << hex_value(item.opcode, 4U) << '\n';
    }
    output << "\nBranches:\n";
    for (const auto& item : report.branches) {
        output << "  " << hex_value(item.instruction_address) << " -> "
               << hex_value(item.target) << (item.taken ? " taken" : " not-taken") << '\n';
    }
    output << "\nMemory reads/writes: " << report.memory_reads.size() << "/"
           << report.memory_writes.size() << '\n';
    for (const auto& item : report.memory_reads) {
        output << "  read " << hex_value(item.instruction_address) << " -> "
               << hex_value(item.address) << " (" << static_cast<unsigned>(item.width_bytes)
               << " bytes)\n";
    }
    output << "\nIndirect targets:\n";
    for (const auto& item : report.indirect_targets) {
        output << "  " << hex_value(item.instruction_address) << " -> "
               << hex_value(item.target) << '\n';
    }
    output << "\nNewly resolved:\n";
    for (const auto& item : report.newly_resolved) {
        output << "  " << hex_value(item.instruction_address) << " " << item.static_kind
               << " = " << hex_value(item.observed_value) << '\n';
    }
    output << "Still unresolved: " << report.still_unresolved.size() << '\n';
    return output.str();
}

} // namespace oasis::tools
