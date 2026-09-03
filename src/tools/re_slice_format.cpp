#include "tools/re_slice_decoder.hpp"

#include <iomanip>
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
    for (const char character : value) {
        if (character == '"' || character == '\\') escaped += '\\';
        escaped += character;
    }
    return escaped;
}

} // namespace

std::string flow_kind_name(FlowKind kind) {
    switch (kind) {
    case FlowKind::none: return "none";
    case FlowKind::direct_branch: return "direct_branch";
    case FlowKind::direct_call: return "direct_call";
    case FlowKind::direct_jump: return "direct_jump";
    case FlowKind::indirect_call: return "indirect_call";
    case FlowKind::indirect_jump: return "indirect_jump";
    case FlowKind::return_instruction: return "return";
    case FlowKind::unsupported: return "unsupported";
    }
    return "unknown";
}

std::string memory_kind_name(MemoryKind kind) {
    return kind == MemoryKind::rom ? "rom" : kind == MemoryKind::ram ? "ram" : "other";
}

std::string memory_access_name(MemoryAccess access) {
    switch (access) {
    case MemoryAccess::read: return "read";
    case MemoryAccess::write: return "write";
    case MemoryAccess::address: return "address";
    case MemoryAccess::unknown: return "unknown";
    }
    return "unknown";
}

std::string slice_to_json(const DecodedSlice& slice) {
    std::ostringstream output;
    output << "{\"schema\":\"oasis.m68k.re-slice.v1\",\"entry_point\":\""
           << hex_value(slice.entry) << "\",\"range_end\":\"" << hex_value(slice.range_end)
           << "\",\"instructions\":[";
    for (std::size_t i = 0; i < slice.instructions.size(); ++i) {
        const auto& instruction = slice.instructions[i];
        if (i != 0U) output << ',';
        output << "{\"address\":\"" << hex_value(instruction.address)
               << "\",\"opcode\":\"" << hex_value(instruction.opcode, 4U)
               << "\",\"bytes\":\"";
        for (const auto byte : instruction.bytes) {
            output << std::hex << std::setfill('0') << std::setw(2)
                   << static_cast<unsigned>(byte);
        }
        output << "\",\"mnemonic\":\"" << json_escape(instruction.mnemonic)
               << "\",\"supported\":" << (instruction.supported ? "true" : "false")
               << ",\"flow\":\"" << flow_kind_name(instruction.flow) << '\"';
        if (instruction.direct_target) {
            output << ",\"direct_target\":\"" << hex_value(*instruction.direct_target) << '\"';
        }
        output << ",\"immediate_constants\":[";
        for (std::size_t j = 0; j < instruction.immediate_constants.size(); ++j) {
            if (j != 0U) output << ',';
            const auto& value = instruction.immediate_constants[j];
            output << "{\"value\":\"" << hex_value(value.value)
                   << "\",\"width_bytes\":" << static_cast<unsigned>(value.width_bytes) << '}';
        }
        output << "],\"memory_references\":[";
        for (std::size_t j = 0; j < instruction.memory_references.size(); ++j) {
            if (j != 0U) output << ',';
            const auto& reference = instruction.memory_references[j];
            output << "{\"address\":\"" << hex_value(reference.address)
                   << "\",\"width_bytes\":" << static_cast<unsigned>(reference.width_bytes)
                   << ",\"kind\":\"" << memory_kind_name(reference.kind)
                   << "\",\"access\":\"" << memory_access_name(reference.access) << "\"}";
        }
        output << "],\"unresolved_memory_references\":[";
        for (std::size_t j = 0; j < instruction.unresolved_memory_references.size(); ++j) {
            if (j != 0U) output << ',';
            const auto& reference = instruction.unresolved_memory_references[j];
            output << "{\"mode\":" << static_cast<unsigned>(reference.mode)
                   << ",\"register\":" << static_cast<unsigned>(reference.register_index)
                   << ",\"reason\":\"" << json_escape(reference.reason) << "\"}";
        }
        output << "],\"unsupported_addressing\":[";
        for (std::size_t j = 0; j < instruction.unsupported_addressing.size(); ++j) {
            if (j != 0U) output << ',';
            const auto& item = instruction.unsupported_addressing[j];
            output << "{\"mode\":" << static_cast<unsigned>(item.mode)
                   << ",\"register\":" << static_cast<unsigned>(item.register_index)
                   << ",\"reason\":\"" << json_escape(item.reason) << "\"}";
        }
        output << "]}";
    }
    output << "],\"basic_blocks\":[";
    for (std::size_t i = 0; i < slice.basic_blocks.size(); ++i) {
        if (i != 0U) output << ',';
        const auto& block = slice.basic_blocks[i];
        output << "{\"start\":\"" << hex_value(block.start)
               << "\",\"end\":\"" << hex_value(block.end) << "\",\"instructions\":[";
        for (std::size_t j = 0; j < block.instruction_addresses.size(); ++j) {
            if (j != 0U) output << ',';
            output << '\"' << hex_value(block.instruction_addresses[j]) << '\"';
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
        output << '\"' << hex_value(slice.unsupported_instruction_addresses[i]) << '\"';
    }
    output << "]}";
    return output.str();
}

std::string slice_to_text(const DecodedSlice& slice) {
    std::ostringstream output;
    std::size_t direct_calls = 0;
    std::size_t direct_branches = 0;
    std::size_t direct_jumps = 0;
    std::size_t rom_refs = 0;
    std::size_t ram_refs = 0;
    std::size_t unresolved_memory_refs = 0;
    std::size_t unsupported_addressing = 0;
    std::set<std::uint32_t> immediate_addresses;
    for (const auto& edge : slice.control_flow) {
        if (edge.kind == FlowKind::direct_call) ++direct_calls;
        if (edge.kind == FlowKind::direct_branch) ++direct_branches;
        if (edge.kind == FlowKind::direct_jump) ++direct_jumps;
    }
    for (const auto& instruction : slice.instructions) {
        if (!instruction.immediate_constants.empty()) immediate_addresses.insert(instruction.address);
        unresolved_memory_refs += instruction.unresolved_memory_references.size();
        unsupported_addressing += instruction.unsupported_addressing.size();
        for (const auto& reference : instruction.memory_references) {
            if (reference.kind == MemoryKind::rom) ++rom_refs;
            if (reference.kind == MemoryKind::ram) ++ram_refs;
        }
    }
    output << "M68K bounded RE slice\nentry=" << hex_value(slice.entry)
           << " range=[" << hex_value(slice.entry) << ", " << hex_value(slice.range_end)
           << ")\ninstructions=" << slice.instructions.size()
           << " basic_blocks=" << slice.basic_blocks.size()
           << " direct_branches=" << direct_branches
           << " direct_calls=" << direct_calls
           << " direct_jumps=" << direct_jumps
           << " unresolved=" << slice.unresolved_control_flow.size()
           << " unsupported=" << slice.unsupported_instruction_addresses.size()
           << "\nimmediate-bearing instructions=" << immediate_addresses.size()
           << " absolute ROM references=" << rom_refs
           << " absolute RAM references=" << ram_refs << '\n';
    output << "unresolved memory references=" << unresolved_memory_refs
           << " unsupported addressing=" << unsupported_addressing << '\n';
    output << "\nBasic blocks (first/last window):\n";
    const auto block_window = std::min<std::size_t>(8U, slice.basic_blocks.size());
    for (std::size_t i = 0; i < block_window; ++i) {
        const auto& block = slice.basic_blocks[i];
        output << "  " << hex_value(block.start) << ".." << hex_value(block.end)
               << " (" << block.instruction_addresses.size() << " instructions)\n";
    }
    if (slice.basic_blocks.size() > 16U) {
        output << "  ... " << slice.basic_blocks.size() - 16U << " blocks omitted; see JSON\n";
    }
    const auto last_block = slice.basic_blocks.size() > 8U ? slice.basic_blocks.size() - 8U : block_window;
    for (std::size_t i = last_block; i < slice.basic_blocks.size(); ++i) {
        const auto& block = slice.basic_blocks[i];
        output << "  " << hex_value(block.start) << ".." << hex_value(block.end)
               << " (" << block.instruction_addresses.size() << " instructions)\n";
    }
    output << "\nDirect control flow (first/last window):\n";
    const auto edge_window = std::min<std::size_t>(12U, slice.control_flow.size());
    for (std::size_t i = 0; i < edge_window; ++i) {
        const auto& edge = slice.control_flow[i];
        output << "  " << hex_value(edge.source) << " -> " << hex_value(edge.target)
               << " " << flow_kind_name(edge.kind) << '\n';
    }
    if (slice.control_flow.size() > 24U) {
        output << "  ... " << slice.control_flow.size() - 24U << " edges omitted; see JSON\n";
    }
    const auto last_edge = slice.control_flow.size() > 12U ? slice.control_flow.size() - 12U : edge_window;
    for (std::size_t i = last_edge; i < slice.control_flow.size(); ++i) {
        const auto& edge = slice.control_flow[i];
        output << "  " << hex_value(edge.source) << " -> " << hex_value(edge.target)
               << " " << flow_kind_name(edge.kind) << '\n';
    }
    output << "\nUnresolved control flow: " << slice.unresolved_control_flow.size() << '\n';
    for (const auto& item : slice.unresolved_control_flow) {
        output << "  " << hex_value(item.address) << " opcode=" << hex_value(item.opcode, 4U)
               << " " << flow_kind_name(item.kind) << '\n';
    }
    output << "Unsupported instruction addresses: ";
    for (std::size_t i = 0; i < slice.unsupported_instruction_addresses.size(); ++i) {
        if (i != 0U) output << ", ";
        output << hex_value(slice.unsupported_instruction_addresses[i]);
    }
    output << '\n';
    return output.str();
}

} // namespace oasis::tools
