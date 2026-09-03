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
        }
    }
    output << "\nInterpretation boundary: signatures and byte/CFG correspondence only;"
              " no semantic conclusions are emitted.\n";
    return output.str();
}

} // namespace oasis::tools
