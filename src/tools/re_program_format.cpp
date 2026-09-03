#include "tools/re_program.hpp"

#include <iomanip>
#include <map>
#include <set>
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

void append_instruction(std::ostringstream& output, const DecodedInstruction& instruction) {
    output << "{\"address\":\"" << hex_value(instruction.address)
           << "\",\"opcode\":\"" << hex_value(instruction.opcode, 4U) << "\",\"bytes\":\"";
    for (const auto byte : instruction.bytes) {
        output << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned>(byte);
    }
    output << "\",\"mnemonic\":\"" << json_escape(instruction.mnemonic)
           << "\",\"supported\":" << (instruction.supported ? "true" : "false")
           << ",\"flow\":\"" << flow_kind_name(instruction.flow) << '"';
    if (instruction.direct_target) {
        output << ",\"direct_target\":\"" << hex_value(*instruction.direct_target) << '"';
    }
    output << ",\"immediate_constants\":[";
    for (std::size_t i = 0; i < instruction.immediate_constants.size(); ++i) {
        if (i != 0U) output << ',';
        const auto& value = instruction.immediate_constants[i];
        output << "{\"value\":\"" << hex_value(value.value)
               << "\",\"width_bytes\":" << static_cast<unsigned>(value.width_bytes) << '}';
    }
    output << "],\"memory_references\":[";
    for (std::size_t i = 0; i < instruction.memory_references.size(); ++i) {
        if (i != 0U) output << ',';
        const auto& reference = instruction.memory_references[i];
        output << "{\"address\":\"" << hex_value(reference.address)
               << "\",\"width_bytes\":" << static_cast<unsigned>(reference.width_bytes)
               << ",\"kind\":\"" << memory_kind_name(reference.kind)
               << "\",\"access\":\"" << memory_access_name(reference.access) << "\"}";
    }
    output << "],\"unresolved_memory_references\":[";
    for (std::size_t i = 0; i < instruction.unresolved_memory_references.size(); ++i) {
        if (i != 0U) output << ',';
        const auto& reference = instruction.unresolved_memory_references[i];
        output << "{\"mode\":" << static_cast<unsigned>(reference.mode)
               << ",\"register\":" << static_cast<unsigned>(reference.register_index)
               << ",\"reason\":\"" << json_escape(reference.reason) << "\"}";
    }
    output << "],\"unsupported_addressing\":[";
    for (std::size_t i = 0; i < instruction.unsupported_addressing.size(); ++i) {
        if (i != 0U) output << ',';
        const auto& item = instruction.unsupported_addressing[i];
        output << "{\"mode\":" << static_cast<unsigned>(item.mode)
               << ",\"register\":" << static_cast<unsigned>(item.register_index)
               << ",\"reason\":\"" << json_escape(item.reason) << "\"}";
    }
    output << "]}";
}

void append_slice(std::ostringstream& output, const DecodedSlice& slice) {
    output << "\"range_end\":\"" << hex_value(slice.range_end) << "\",\"instructions\":[";
    for (std::size_t i = 0; i < slice.instructions.size(); ++i) {
        if (i != 0U) output << ',';
        append_instruction(output, slice.instructions[i]);
    }
    output << "],\"basic_blocks\":[";
    for (std::size_t i = 0; i < slice.basic_blocks.size(); ++i) {
        if (i != 0U) output << ',';
        const auto& block = slice.basic_blocks[i];
        output << "{\"start\":\"" << hex_value(block.start) << "\",\"end\":\""
               << hex_value(block.end) << "\",\"instructions\":[";
        for (std::size_t j = 0; j < block.instruction_addresses.size(); ++j) {
            if (j != 0U) output << ',';
            output << '"' << hex_value(block.instruction_addresses[j]) << '"';
        }
        output << "]}";
    }
    output << "],\"direct_control_flow\":[";
    for (std::size_t i = 0; i < slice.control_flow.size(); ++i) {
        if (i != 0U) output << ',';
        const auto& edge = slice.control_flow[i];
        output << "{\"source\":\"" << hex_value(edge.source)
               << "\",\"target\":\"" << hex_value(edge.target)
               << "\",\"kind\":\"" << flow_kind_name(edge.kind) << "\"}";
    }
    output << "],\"unresolved_control_flow\":[";
    for (std::size_t i = 0; i < slice.unresolved_control_flow.size(); ++i) {
        if (i != 0U) output << ',';
        const auto& item = slice.unresolved_control_flow[i];
        output << "{\"address\":\"" << hex_value(item.address)
               << "\",\"opcode\":\"" << hex_value(item.opcode, 4U)
               << "\",\"kind\":\"" << flow_kind_name(item.kind) << "\"}";
    }
    output << "],\"unsupported_instruction_addresses\":[";
    for (std::size_t i = 0; i < slice.unsupported_instruction_addresses.size(); ++i) {
        if (i != 0U) output << ',';
        output << '"' << hex_value(slice.unsupported_instruction_addresses[i]) << '"';
    }
    output << ']';
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

std::string program_to_json(const MultiSliceReport& report) {
    std::ostringstream output;
    output << "{\"schema\":\"oasis.m68k.re-program.v1\",\"functions\":[";
    for (std::size_t i = 0; i < report.functions.size(); ++i) {
        if (i != 0U) output << ',';
        const auto& function = report.functions[i];
        output << "{\"entry_point\":\"" << hex_value(function.entry)
               << "\",\"boundary\":\"" << boundary_status_name(function.boundary) << '"';
        if (function.boundary_end) output << ",\"boundary_end\":\"" << hex_value(*function.boundary_end) << '"';
        else output << ",\"boundary_end\":null";
        output << ",\"slice\":{";
        append_slice(output, function.slice);
        output << "}}";
    }
    output << "],\"call_graph\":{";
    output << "\"caller_to_callee\":";
    append_array(output, report.function_call_edges, [](auto& out, const auto& edge) {
        out << "{\"caller\":\"" << hex_value(edge.caller_entry)
            << "\",\"callee\":\"" << hex_value(edge.callee_entry) << "\",\"call_sites\":[";
        for (std::size_t i = 0; i < edge.call_sites.size(); ++i) {
            if (i != 0U) out << ',';
            out << '"' << hex_value(edge.call_sites[i]) << '"';
        }
        out << "]}";
    });
    output << ",\"basic_block_calls\":";
    append_array(output, report.direct_call_sites, [](auto& out, const auto& call) {
        out << "{\"caller\":\"" << hex_value(call.caller_entry)
            << "\",\"block\":\"" << hex_value(call.block_start)
            << "\",\"instruction\":\"" << hex_value(call.instruction_address)
            << "\",\"target\":\"" << hex_value(call.target)
            << "\",\"target_analyzed\":" << (call.target_analyzed ? "true" : "false") << '}';
    });
    output << ",\"unresolved_or_indirect\":";
    append_array(output, report.unresolved_control_flow, [](auto& out, const auto& item) {
        out << "{\"function\":\"" << hex_value(item.function_entry)
            << "\",\"block\":\"" << hex_value(item.block_start)
            << "\",\"instruction\":\"" << hex_value(item.flow.address)
            << "\",\"opcode\":\"" << hex_value(item.flow.opcode, 4U)
            << "\",\"kind\":\"" << flow_kind_name(item.flow.kind) << "\"}";
    });
    output << "},\"confirmed_memory_references\":";
    append_array(output, report.confirmed_memory_references, [](auto& out, const auto& item) {
        out << "{\"function\":\"" << hex_value(item.function_entry)
            << "\",\"slice_range_end\":\"" << hex_value(item.slice_range_end)
            << "\",\"block\":\"" << hex_value(item.block_start)
            << "\",\"instruction\":\"" << hex_value(item.instruction_address)
            << "\",\"address\":\"" << hex_value(item.reference.address)
            << "\",\"width_bytes\":" << static_cast<unsigned>(item.reference.width_bytes)
            << ",\"kind\":\"" << memory_kind_name(item.reference.kind)
            << "\",\"access\":\"" << memory_access_name(item.reference.access) << "\"}";
    });
    output << ",\"unresolved_memory_references\":";
    append_array(output, report.unresolved_memory_references, [](auto& out, const auto& item) {
        out << "{\"function\":\"" << hex_value(item.function_entry)
            << "\",\"slice_range_end\":\"" << hex_value(item.slice_range_end)
            << "\",\"block\":\"" << hex_value(item.block_start)
            << "\",\"instruction\":\"" << hex_value(item.instruction_address)
            << "\",\"mode\":" << static_cast<unsigned>(item.reference.mode)
            << ",\"register\":" << static_cast<unsigned>(item.reference.register_index)
            << ",\"reason\":\"" << json_escape(item.reference.reason) << "\"}";
    });
    output << ",\"unsupported_addressing\":";
    append_array(output, report.unsupported_addressing, [](auto& out, const auto& item) {
        out << "{\"function\":\"" << hex_value(item.function_entry)
            << "\",\"slice_range_end\":\"" << hex_value(item.slice_range_end)
            << "\",\"block\":\"" << hex_value(item.block_start)
            << "\",\"instruction\":\"" << hex_value(item.instruction_address)
            << "\",\"mode\":" << static_cast<unsigned>(item.reference.mode)
            << ",\"register\":" << static_cast<unsigned>(item.reference.register_index)
            << ",\"reason\":\"" << json_escape(item.reference.reason) << "\"}";
    });
    output << ",\"unsupported_instructions\":";
    append_array(output, report.unsupported_instructions, [](auto& out, const auto& item) {
        out << "{\"function\":\"" << hex_value(item.function_entry)
            << "\",\"slice_range_end\":\"" << hex_value(item.slice_range_end)
            << "\",\"block\":\"" << hex_value(item.block_start)
            << "\",\"instruction\":\"" << hex_value(item.instruction_address)
            << "\",\"opcode\":\"" << hex_value(item.opcode, 4U) << "\"}";
    });
    output << '}';
    return output.str();
}

std::string program_to_text(const MultiSliceReport& report) {
    std::ostringstream output;
    std::size_t instructions = 0;
    std::size_t blocks = 0;
    for (const auto& function : report.functions) {
        instructions += function.slice.instructions.size();
        blocks += function.slice.basic_blocks.size();
    }
    output << "M68K bounded multi-slice RE report\n"
           << "functions=" << report.functions.size() << " instructions=" << instructions
           << " basic_blocks=" << blocks << " direct_call_sites=" << report.direct_call_sites.size()
           << " caller_to_callee=" << report.function_call_edges.size()
           << " confirmed_memory_refs=" << report.confirmed_memory_references.size()
           << " unresolved_memory_refs=" << report.unresolved_memory_references.size()
           << " unsupported_addressing=" << report.unsupported_addressing.size()
           << " unsupported_instructions=" << report.unsupported_instructions.size() << '\n';
    output << "\nFunctions:\n";
    for (const auto& function : report.functions) {
        output << "  " << hex_value(function.entry) << " range=["
               << hex_value(function.entry) << ", " << hex_value(function.range_end)
               << ") boundary=" << boundary_status_name(function.boundary);
        if (function.boundary_end) output << " end=" << hex_value(*function.boundary_end);
        output << " instructions=" << function.slice.instructions.size()
               << " blocks=" << function.slice.basic_blocks.size() << '\n';
    }
    output << "\nCaller -> callee (direct only):\n";
    for (const auto& edge : report.function_call_edges) {
        output << "  " << hex_value(edge.caller_entry) << " -> " << hex_value(edge.callee_entry)
               << " call_sites=" << edge.call_sites.size() << '\n';
    }
    output << "\nDirect call sites (block-bound):\n";
    for (const auto& call : report.direct_call_sites) {
        output << "  " << hex_value(call.caller_entry) << '/' << hex_value(call.block_start)
               << '/' << hex_value(call.instruction_address) << " -> " << hex_value(call.target)
               << (call.target_analyzed ? " analyzed" : " outside-set") << '\n';
    }
    output << "\nUnresolved or indirect control flow: " << report.unresolved_control_flow.size() << '\n';
    for (const auto& item : report.unresolved_control_flow) {
        output << "  " << hex_value(item.function_entry) << '/' << hex_value(item.block_start)
               << '/' << hex_value(item.flow.address) << " " << flow_kind_name(item.flow.kind) << '\n';
    }
    output << "\nMemory evidence: confirmed=" << report.confirmed_memory_references.size()
           << " unresolved=" << report.unresolved_memory_references.size()
           << " unsupported_addressing=" << report.unsupported_addressing.size() << '\n';
    output << "Unsupported opcodes: " << report.unsupported_instructions.size() << '\n';
    return output.str();
}

} // namespace oasis::tools
