#include "tools/re_diff.hpp"

#include <algorithm>
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

void append_fingerprint(std::ostringstream& output, const RomIdentity& identity) {
    output << "{\"display_name\":\"" << json_escape(identity.display_name)
           << "\",\"status\":\"" << to_string(identity.status)
           << "\",\"size\":" << identity.fingerprint.size
           << ",\"crc32\":\"" << hex_value(identity.fingerprint.crc32)
           << "\",\"sha1\":\"" << identity.fingerprint.sha1
           << "\",\"sha256\":\"" << identity.fingerprint.sha256
           << "\",\"sega_checksum\":\"" << hex_value(identity.fingerprint.calculated_sega_checksum, 4U)
           << "\",\"sega_checksum_valid\":"
           << (identity.fingerprint.sega_checksum_valid ? "true" : "false") << '}';
}

void append_changed_blocks(std::ostringstream& output, const std::vector<ChangedBlock>& blocks) {
    output << '[';
    for (std::size_t index = 0; index < blocks.size(); ++index) {
        if (index != 0U) output << ',';
        const auto& block = blocks[index];
        output << "{\"ordinal\":" << block.ordinal;
        if (block.retail_start) output << ",\"retail_start\":\"" << hex_value(*block.retail_start) << '"';
        else output << ",\"retail_start\":null";
        if (block.beta_start) output << ",\"beta_start\":\"" << hex_value(*block.beta_start) << '"';
        else output << ",\"beta_start\":null";
        output << '}';
    }
    output << ']';
}

void append_strings(std::ostringstream& output, const std::vector<std::string>& values) {
    output << '[';
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0U) output << ',';
        output << '"' << json_escape(values[index]) << '"';
    }
    output << ']';
}

void append_instruction(std::ostringstream& output, const DecodedInstruction& instruction) {
    output << "{\"address\":\"" << hex_value(instruction.address)
           << "\",\"opcode\":\"" << hex_value(instruction.opcode, 4U) << "\",\"bytes\":\"";
    for (const auto byte : instruction.bytes) {
        output << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned>(byte);
    }
    output << std::dec;
    output << "\",\"mnemonic\":\"" << json_escape(instruction.mnemonic)
           << "\",\"supported\":" << (instruction.supported ? "true" : "false")
           << ",\"flow\":\"" << flow_kind_name(instruction.flow) << '\"';
    if (instruction.direct_target) output << ",\"direct_target\":\"" << hex_value(*instruction.direct_target) << '\"';
    if (instruction.branch_condition_code) {
        output << ",\"branch_condition_code\":" << static_cast<unsigned>(*instruction.branch_condition_code);
    }
    output << ",\"addressing_modes\":";
    append_strings(output, instruction.addressing_modes);
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

void append_optional_block(std::ostringstream& output, const std::optional<BasicBlock>& block) {
    if (!block) {
        output << "null";
        return;
    }
    output << "{\"start\":\"" << hex_value(block->start) << "\",\"end\":\""
           << hex_value(block->end) << "\",\"instructions\":[";
    for (std::size_t i = 0; i < block->instruction_addresses.size(); ++i) {
        if (i != 0U) output << ',';
        output << '"' << hex_value(block->instruction_addresses[i]) << '"';
    }
    output << "]}";
}

void append_edges(std::ostringstream& output, const std::vector<ControlFlowEdge>& edges) {
    output << '[';
    for (std::size_t i = 0; i < edges.size(); ++i) {
        if (i != 0U) output << ',';
        const auto& edge = edges[i];
        output << "{\"source\":\"" << hex_value(edge.source)
               << "\",\"target\":\"" << hex_value(edge.target)
               << "\",\"kind\":\"" << flow_kind_name(edge.kind) << "\"}";
    }
    output << ']';
}

void append_unresolved_edges(std::ostringstream& output, const std::vector<UnresolvedControlFlow>& edges) {
    output << '[';
    for (std::size_t i = 0; i < edges.size(); ++i) {
        if (i != 0U) output << ',';
        const auto& edge = edges[i];
        output << "{\"address\":\"" << hex_value(edge.address)
               << "\",\"opcode\":\"" << hex_value(edge.opcode, 4U)
               << "\",\"kind\":\"" << flow_kind_name(edge.kind) << "\"}";
    }
    output << ']';
}

void append_block_detail(std::ostringstream& output, const BlockDetail& detail) {
    output << std::dec;
    output << "{\"ordinal\":" << detail.ordinal << ",\"retail_block\":";
    append_optional_block(output, detail.retail_block);
    output << ",\"beta_block\":";
    append_optional_block(output, detail.beta_block);
    output << ",\"retail_predecessors\":";
    append_edges(output, detail.retail_predecessors);
    output << ",\"beta_predecessors\":";
    append_edges(output, detail.beta_predecessors);
    output << ",\"retail_fallthrough_predecessors\":";
    append_edges(output, detail.retail_fallthrough_predecessors);
    output << ",\"beta_fallthrough_predecessors\":";
    append_edges(output, detail.beta_fallthrough_predecessors);
    output << ",\"retail_successors\":";
    append_edges(output, detail.retail_successors);
    output << ",\"beta_successors\":";
    append_edges(output, detail.beta_successors);
    output << ",\"retail_fallthrough_successors\":";
    append_edges(output, detail.retail_fallthrough_edges);
    output << ",\"beta_fallthrough_successors\":";
    append_edges(output, detail.beta_fallthrough_edges);
    output << ",\"retail_unresolved_successors\":";
    append_unresolved_edges(output, detail.retail_unresolved_successors);
    output << ",\"beta_unresolved_successors\":";
    append_unresolved_edges(output, detail.beta_unresolved_successors);
    output << ",\"topology_differences\":";
    append_strings(output, detail.topology_differences);
    output << ",\"instruction_differences\":[";
    for (std::size_t i = 0; i < detail.instruction_differences.size(); ++i) {
        if (i != 0U) output << ',';
        const auto& difference = detail.instruction_differences[i];
        output << "{\"retail_address\":";
        if (difference.retail_address) output << '"' << hex_value(*difference.retail_address) << '"'; else output << "null";
        output << ",\"beta_address\":";
        if (difference.beta_address) output << '"' << hex_value(*difference.beta_address) << '"'; else output << "null";
        output << ",\"classifications\":[";
        for (std::size_t j = 0; j < difference.classifications.size(); ++j) {
            if (j != 0U) output << ',';
            output << '"' << instruction_diff_kind_name(difference.classifications[j]) << '"';
        }
        output << "],\"retail_instruction\":";
        if (difference.retail_instruction) append_instruction(output, *difference.retail_instruction); else output << "null";
        output << ",\"beta_instruction\":";
        if (difference.beta_instruction) append_instruction(output, *difference.beta_instruction); else output << "null";
        output << '}';
    }
    output << "]}";
}

void append_block_details(std::ostringstream& output, const std::vector<BlockDetail>& details) {
    output << '[';
    for (std::size_t i = 0; i < details.size(); ++i) {
        if (i != 0U) output << ',';
        append_block_detail(output, details[i]);
    }
    output << ']';
}

std::string bytes_hex(const std::vector<std::uint8_t>& bytes) {
    std::ostringstream output;
    for (const auto byte : bytes) output << std::hex << std::setfill('0') << std::setw(2)
                                         << static_cast<unsigned>(byte);
    return output.str();
}

void append_instruction_text(std::ostringstream& output, const DecodedInstruction& instruction) {
    output << hex_value(instruction.address) << " opcode=" << hex_value(instruction.opcode, 4U)
           << " bytes=" << bytes_hex(instruction.bytes) << " " << instruction.mnemonic
           << " flow=" << flow_kind_name(instruction.flow);
    if (instruction.direct_target) output << " target=" << hex_value(*instruction.direct_target);
    if (instruction.branch_condition_code) {
        output << " condition_code=" << static_cast<unsigned>(*instruction.branch_condition_code);
    }
    if (!instruction.addressing_modes.empty()) {
        output << " ea=";
        for (std::size_t i = 0; i < instruction.addressing_modes.size(); ++i) {
            if (i != 0U) output << ',';
            output << instruction.addressing_modes[i];
        }
    }
    if (!instruction.immediate_constants.empty()) {
        output << " immediates=";
        for (std::size_t i = 0; i < instruction.immediate_constants.size(); ++i) {
            if (i != 0U) output << ',';
            output << hex_value(instruction.immediate_constants[i].value);
        }
    }
    if (!instruction.memory_references.empty()) {
        output << " refs=";
        for (std::size_t i = 0; i < instruction.memory_references.size(); ++i) {
            if (i != 0U) output << ',';
            output << hex_value(instruction.memory_references[i].address);
        }
    }
}

void append_edge_text(std::ostringstream& output, const std::vector<ControlFlowEdge>& edges) {
    for (const auto& edge : edges) {
        output << ' ' << hex_value(edge.source) << "->" << hex_value(edge.target)
               << ':' << flow_kind_name(edge.kind);
    }
}

void append_detail_text(std::ostringstream& output, const BlockDetail& detail) {
    output << "        block_detail ordinal=" << detail.ordinal << " retail=";
    if (detail.retail_block) output << hex_value(detail.retail_block->start) << ".." << hex_value(detail.retail_block->end);
    else output << "none";
    output << " beta=";
    if (detail.beta_block) output << hex_value(detail.beta_block->start) << ".." << hex_value(detail.beta_block->end);
    else output << "none";
    output << "\n          predecessors retail:";
    append_edge_text(output, detail.retail_predecessors);
    append_edge_text(output, detail.retail_fallthrough_predecessors);
    output << " beta:";
    append_edge_text(output, detail.beta_predecessors);
    append_edge_text(output, detail.beta_fallthrough_predecessors);
    output << "\n          successors retail:";
    append_edge_text(output, detail.retail_successors);
    append_edge_text(output, detail.retail_fallthrough_edges);
    output << " beta:";
    append_edge_text(output, detail.beta_successors);
    append_edge_text(output, detail.beta_fallthrough_edges);
    if (!detail.retail_unresolved_successors.empty() || !detail.beta_unresolved_successors.empty()) {
        output << "\n          unresolved_successors retail=" << detail.retail_unresolved_successors.size()
               << " beta=" << detail.beta_unresolved_successors.size();
    }
    if (!detail.topology_differences.empty()) output << "\n          topology=" << detail.topology_differences.front();
    output << "\n          instructions:\n";
    for (const auto& difference : detail.instruction_differences) {
        output << "            [";
        for (std::size_t i = 0; i < difference.classifications.size(); ++i) {
            if (i != 0U) output << ',';
            output << instruction_diff_kind_name(difference.classifications[i]);
        }
        output << "] retail=";
        if (difference.retail_instruction) append_instruction_text(output, *difference.retail_instruction);
        else output << "none";
        output << " | beta=";
        if (difference.beta_instruction) append_instruction_text(output, *difference.beta_instruction);
        else output << "none";
        output << '\n';
    }
}

} // namespace

std::string diff_to_json(const DifferentialReport& report) {
    std::ostringstream output;
    output << "{\"schema\":\"oasis.m68k.re-diff.v1\",\"retail\":";
    append_fingerprint(output, report.retail);
    output << ",\"beta\":";
    append_fingerprint(output, report.beta);
    output << ",\"targets\":[";
    for (std::size_t index = 0; index < report.targets.size(); ++index) {
        if (index != 0U) output << ',';
        const auto& target = report.targets[index];
        output << "{\"retail_entry\":\"" << hex_value(target.target.entry)
               << "\",\"byte_budget\":"
               << (target.target.confirmed_end ? *target.target.confirmed_end - target.target.entry
                                                : target.target.byte_budget)
               << ",\"retail_range_end\":\"" << hex_value(target.retail_range_end)
               << "\",\"beta_same_address_range_end\":\"" << hex_value(target.beta_same_address_range_end)
               << "\",\"retail_instructions\":" << target.retail_instructions
               << ",\"retail_basic_blocks\":" << target.retail_basic_blocks
               << ",\"retail_normalized_opcode_signature\":";
        append_strings(output, target.retail_normalized_opcode_signature);
        output << ",\"same_address\":{\"beta_normalized_opcode_signature\":";
        append_strings(output, target.beta_same_address_normalized_opcode_signature);
        output << ",\"match\":\"" << match_kind_name(target.same_address_match)
               << "\",\"unmatched_retail_instructions\":" << target.unmatched_retail_instructions
               << ",\"unmatched_beta_instructions\":" << target.unmatched_beta_instructions
               << ",\"changed_blocks\":";
        append_changed_blocks(output, target.same_address_changed_blocks);
        output << ",\"changed_block_details\":";
        append_block_details(output, target.same_address_changed_block_details);
        output << "},\"analogs\":[";
        for (std::size_t analog = 0; analog < target.analogs.size(); ++analog) {
            if (analog != 0U) output << ',';
            const auto& candidate = target.analogs[analog];
            output << "{\"beta_entry\":\"" << hex_value(candidate.beta_entry)
                   << "\",\"match\":\"" << match_kind_name(candidate.match)
                   << "\",\"matching_instructions\":" << candidate.matching_instructions
                   << ",\"matching_blocks\":" << candidate.matching_blocks
                   << ",\"changed_blocks\":";
            append_changed_blocks(output, candidate.changed_blocks);
            output << ",\"changed_block_details\":";
            append_block_details(output, candidate.changed_block_details);
            output << ",\"beta_normalized_opcode_signature\":";
            append_strings(output, candidate.beta_normalized_opcode_signature);
            output << '}';
        }
        output << "]}";
    }
    output << "]}";
    return output.str();
}

std::string diff_to_text(const DifferentialReport& report) {
    std::ostringstream output;
    output << "M68K bounded revision differential report\n"
           << "schema=oasis.m68k.re-diff.v1\n"
           << "retail=" << report.retail.display_name << " crc32="
           << hex_value(report.retail.fingerprint.crc32) << " sha256=" << report.retail.fingerprint.sha256 << '\n'
           << "beta=" << report.beta.display_name << " crc32="
           << hex_value(report.beta.fingerprint.crc32) << " sha256=" << report.beta.fingerprint.sha256 << '\n';
    output << "\nTargets:\n";
    for (const auto& target : report.targets) {
        const auto byte_budget = target.target.confirmed_end
                                     ? *target.target.confirmed_end - target.target.entry
                                     : target.target.byte_budget;
        output << "  " << hex_value(target.target.entry) << " bytes=" << byte_budget
               << " instructions=" << target.retail_instructions
               << " blocks=" << target.retail_basic_blocks
               << " normalized_signature_instructions=" << target.retail_normalized_opcode_signature.size()
               << " same_address=" << match_kind_name(target.same_address_match)
               << " changed_blocks=" << target.same_address_changed_blocks.size()
               << " unmatched_retail=" << target.unmatched_retail_instructions
               << " unmatched_beta=" << target.unmatched_beta_instructions << '\n';
        output << "    analogs:\n";
        if (target.analogs.empty()) output << "      none\n";
        for (const auto& analog : target.analogs) {
            output << "      beta " << hex_value(analog.beta_entry)
                   << " " << match_kind_name(analog.match)
                   << " matching_instructions=" << analog.matching_instructions
                   << " matching_blocks=" << analog.matching_blocks
                   << " changed_blocks=" << analog.changed_blocks.size() << '\n';
            const auto shown = std::min<std::size_t>(analog.changed_blocks.size(), 8U);
            for (std::size_t index = 0; index < shown; ++index) {
                const auto& block = analog.changed_blocks[index];
                output << "        changed_block " << block.ordinal << " retail=";
                if (block.retail_start) output << hex_value(*block.retail_start);
                else output << "none";
                output << " beta=";
                if (block.beta_start) output << hex_value(*block.beta_start);
                else output << "none";
                output << '\n';
            }
            if (analog.changed_blocks.size() > shown) {
                output << "        ... " << analog.changed_blocks.size() - shown
                       << " changed blocks omitted; see JSON\n";
            }
            for (const auto& detail : analog.changed_block_details) append_detail_text(output, detail);
        }
    }
    output << "\nInterpretation boundary: signatures and byte/CFG correspondence only;"
              " no semantic conclusions are emitted.\n";
    return output.str();
}

} // namespace oasis::tools
