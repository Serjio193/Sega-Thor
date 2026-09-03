#include "tools/re_diff.hpp"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <tuple>

namespace oasis::tools {
namespace {

constexpr std::size_t kMaxAnalogCandidatesScanned = 32768U;
constexpr std::size_t kMaxReportedAnalogs = 8U;

std::string hex16(std::uint16_t value) {
    std::ostringstream output;
    output << std::hex << std::setfill('0') << std::setw(4) << value;
    return output.str();
}

std::uint16_t read16(std::span<const std::uint8_t> rom, std::uint32_t address) {
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(rom[address]) << 8U) |
                                      rom[address + 1U]);
}

std::uint16_t opcode_shape(std::uint16_t opcode) {
    if ((opcode >> 12U) == 6U) return static_cast<std::uint16_t>(opcode & 0xFF00U);
    if ((opcode & 0xFFC0U) == 0x4E80U || (opcode & 0xFFC0U) == 0x4EC0U) {
        return static_cast<std::uint16_t>(opcode & 0xFFC0U);
    }
    if ((opcode & 0xF100U) == 0x7000U) return static_cast<std::uint16_t>(opcode & 0xFF00U);
    return opcode;
}

std::string instruction_signature(const DecodedInstruction& instruction) {
    std::ostringstream output;
    output << instruction.mnemonic << ':' << flow_kind_name(instruction.flow)
           << ":op=" << hex16(opcode_shape(instruction.opcode))
           << ":len=" << instruction.bytes.size();
    output << ":imm=";
    for (const auto& immediate : instruction.immediate_constants) {
        output << static_cast<unsigned>(immediate.width_bytes) << ',';
    }
    output << ":mem=";
    for (const auto& reference : instruction.memory_references) {
        output << memory_access_name(reference.access) << ':'
               << static_cast<unsigned>(reference.width_bytes) << ':'
               << memory_kind_name(reference.kind) << ',';
    }
    output << ":unresolved=";
    for (const auto& reference : instruction.unresolved_memory_references) {
        output << static_cast<unsigned>(reference.mode) << ':'
               << static_cast<unsigned>(reference.register_index) << ',';
    }
    output << ":unsupported=" << instruction.unsupported_addressing.size();
    return output.str();
}

struct SliceProfile {
    std::vector<std::uint32_t> relative_instruction_addresses;
    std::vector<std::string> instruction_signatures;
    std::vector<std::vector<std::string>> block_signatures;
    std::vector<std::size_t> block_instruction_counts;
    std::vector<std::string> topology;
    std::vector<std::vector<std::uint8_t>> raw_instruction_bytes;
};

std::map<std::uint32_t, std::size_t> block_indexes(const DecodedSlice& slice) {
    std::map<std::uint32_t, std::size_t> result;
    for (std::size_t index = 0; index < slice.basic_blocks.size(); ++index) {
        for (const auto address : slice.basic_blocks[index].instruction_addresses) {
            result[address] = index;
        }
    }
    return result;
}

SliceProfile profile_slice(const DecodedSlice& slice) {
    SliceProfile profile;
    const auto indexes = block_indexes(slice);
    std::map<std::uint32_t, const DecodedInstruction*> instructions;
    for (const auto& instruction : slice.instructions) instructions[instruction.address] = &instruction;
    for (const auto& instruction : slice.instructions) {
        profile.relative_instruction_addresses.push_back(instruction.address - slice.entry);
        profile.instruction_signatures.push_back(instruction_signature(instruction));
        profile.raw_instruction_bytes.push_back(instruction.bytes);
    }
    for (const auto& block : slice.basic_blocks) {
        std::vector<std::string> signatures;
        for (const auto address : block.instruction_addresses) {
            signatures.push_back(instruction_signature(*instructions.at(address)));
        }
        profile.block_instruction_counts.push_back(signatures.size());
        profile.block_signatures.push_back(std::move(signatures));
    }
    for (const auto& edge : slice.control_flow) {
        std::ostringstream output;
        output << "direct:" << indexes.at(edge.source) << ':' << flow_kind_name(edge.kind) << ':';
        const auto target = indexes.find(edge.target);
        if (target == indexes.end()) output << "external";
        else output << target->second;
        profile.topology.push_back(output.str());
    }
    for (const auto& item : slice.unresolved_control_flow) {
        std::ostringstream output;
        output << "unresolved:" << indexes.at(item.address) << ':' << flow_kind_name(item.kind);
        profile.topology.push_back(output.str());
    }
    std::sort(profile.topology.begin(), profile.topology.end());
    return profile;
}

bool same_structure(const SliceProfile& left, const SliceProfile& right) {
    return left.block_instruction_counts == right.block_instruction_counts &&
           left.topology == right.topology;
}

bool same_normalized(const SliceProfile& left, const SliceProfile& right) {
    return same_structure(left, right) && left.instruction_signatures == right.instruction_signatures;
}

bool same_raw(const SliceProfile& left, const SliceProfile& right) {
    return left.relative_instruction_addresses == right.relative_instruction_addresses &&
           left.raw_instruction_bytes == right.raw_instruction_bytes;
}

std::vector<ChangedBlock> changed_blocks(const DecodedSlice& retail, const DecodedSlice& beta) {
    const auto count = std::max(retail.basic_blocks.size(), beta.basic_blocks.size());
    std::vector<ChangedBlock> result;
    for (std::size_t index = 0; index < count; ++index) {
        const auto retail_present = index < retail.basic_blocks.size();
        const auto beta_present = index < beta.basic_blocks.size();
        bool equal = retail_present && beta_present && retail.basic_blocks[index].instruction_addresses.size() ==
                         beta.basic_blocks[index].instruction_addresses.size();
        if (equal) {
            for (std::size_t instruction = 0;
                 instruction < retail.basic_blocks[index].instruction_addresses.size(); ++instruction) {
                const auto retail_address = retail.basic_blocks[index].instruction_addresses[instruction];
                const auto beta_address = beta.basic_blocks[index].instruction_addresses[instruction];
                const auto& retail_item = *std::find_if(retail.instructions.begin(), retail.instructions.end(),
                    [retail_address](const auto& item) { return item.address == retail_address; });
                const auto& beta_item = *std::find_if(beta.instructions.begin(), beta.instructions.end(),
                    [beta_address](const auto& item) { return item.address == beta_address; });
                if (retail_item.bytes != beta_item.bytes) {
                    equal = false;
                    break;
                }
            }
        }
        if (!equal) {
            result.push_back({index, retail_present ? std::optional{retail.basic_blocks[index].start} : std::nullopt,
                              beta_present ? std::optional{beta.basic_blocks[index].start} : std::nullopt});
        }
    }
    return result;
}

MatchKind classify(const DecodedSlice& retail, const DecodedSlice& beta) {
    const auto retail_profile = profile_slice(retail);
    const auto beta_profile = profile_slice(beta);
    if (same_structure(retail_profile, beta_profile) && same_raw(retail_profile, beta_profile)) {
        return MatchKind::exact_match;
    }
    if (same_normalized(retail_profile, beta_profile)) return MatchKind::structural_match;
    if (same_structure(retail_profile, beta_profile)) return MatchKind::changed_blocks;
    return MatchKind::unmatched;
}

std::size_t matching_instructions(const DecodedSlice& retail, const DecodedSlice& beta) {
    const auto left = profile_slice(retail).instruction_signatures;
    const auto right = profile_slice(beta).instruction_signatures;
    const auto count = std::min(left.size(), right.size());
    std::size_t result = 0;
    for (std::size_t index = 0; index < count; ++index) result += left[index] == right[index];
    return result;
}

std::size_t matching_blocks(const DecodedSlice& retail, const DecodedSlice& beta) {
    const auto left = profile_slice(retail).block_signatures;
    const auto right = profile_slice(beta).block_signatures;
    const auto count = std::min(left.size(), right.size());
    std::size_t result = 0;
    for (std::size_t index = 0; index < count; ++index) result += left[index] == right[index];
    return result;
}

std::size_t mismatching_instructions(const DecodedSlice& retail, const DecodedSlice& beta) {
    const auto left = profile_slice(retail).instruction_signatures;
    const auto right = profile_slice(beta).instruction_signatures;
    const auto count = std::max(left.size(), right.size());
    std::size_t result = 0;
    for (std::size_t index = 0; index < count; ++index) {
        if (index >= left.size() || index >= right.size() || left[index] != right[index]) ++result;
    }
    return result;
}

std::size_t target_budget(const DifferentialTarget& target) {
    return target.confirmed_end ? *target.confirmed_end - target.entry : target.byte_budget;
}

DecodedSlice decode_target(std::span<const std::uint8_t> rom, const DifferentialTarget& target,
                           std::uint32_t entry) {
    return decode_m68k_slice(rom, {.entry = entry, .byte_budget = target_budget(target)});
}

bool candidate_entry_prefilter(std::span<const std::uint8_t> beta, std::uint32_t entry,
                               const DecodedInstruction& retail_entry) {
    if (entry + 2U > beta.size()) return false;
    return opcode_shape(read16(beta, entry)) == opcode_shape(retail_entry.opcode);
}

bool has_relative_anchor(std::span<const std::uint8_t> beta, std::uint32_t entry,
                         const DecodedSlice& retail) {
    if (retail.instructions.empty()) return false;
    const auto& first = retail.instructions.front();
    if (!first.direct_target || first.direct_target <= retail.entry) return true;
    const auto relative = *first.direct_target - retail.entry;
    if (entry + relative + 2U > beta.size()) return false;
    const auto retail_target = std::find_if(retail.instructions.begin(), retail.instructions.end(),
        [target = *first.direct_target](const auto& item) { return item.address == target; });
    if (retail_target == retail.instructions.end()) return true;
    return opcode_shape(read16(beta, entry + relative)) == opcode_shape(retail_target->opcode);
}

AnalogCandidate make_candidate(std::uint32_t entry, const DecodedSlice& retail,
                               const DecodedSlice& beta) {
    const auto match = classify(retail, beta);
    const auto beta_profile = profile_slice(beta);
    return {.beta_entry = entry,
            .match = match,
            .matching_instructions = matching_instructions(retail, beta),
            .matching_blocks = matching_blocks(retail, beta),
            .changed_blocks = changed_blocks(retail, beta),
            .beta_normalized_opcode_signature = std::move(beta_profile.instruction_signatures)};
}

int match_rank(MatchKind kind) {
    switch (kind) {
    case MatchKind::exact_match: return 3;
    case MatchKind::structural_match: return 2;
    case MatchKind::changed_blocks: return 1;
    case MatchKind::unmatched: return 0;
    }
    return 0;
}

} // namespace

std::string match_kind_name(MatchKind kind) {
    switch (kind) {
    case MatchKind::exact_match: return "exact_match";
    case MatchKind::structural_match: return "structural_match";
    case MatchKind::changed_blocks: return "changed_blocks";
    case MatchKind::unmatched: return "unmatched";
    }
    return "unmatched";
}

DifferentialReport compare_m68k_revisions(std::span<const std::uint8_t> retail_rom,
                                          std::span<const std::uint8_t> beta_rom,
                                          std::span<const DifferentialTarget> targets) {
    if (retail_rom.empty() || beta_rom.empty()) throw std::invalid_argument("ROM data is empty");
    DifferentialReport report{.retail = identify_rom(retail_rom), .beta = identify_rom(beta_rom)};
    for (const auto& target : targets) {
        if (target.entry & 1U || target_budget(target) < 2U ||
            static_cast<std::size_t>(target.entry) + target_budget(target) > retail_rom.size()) {
            throw std::invalid_argument("invalid retail differential target");
        }
        if (static_cast<std::size_t>(target.entry) + target_budget(target) > beta_rom.size()) {
            throw std::invalid_argument("target does not fit in beta ROM");
        }
        const auto retail = decode_target(retail_rom, target, target.entry);
        const auto beta_same_address = decode_target(beta_rom, target, target.entry);
        const auto retail_profile = profile_slice(retail);
        const auto beta_same_profile = profile_slice(beta_same_address);
        TargetComparison comparison{.target = target,
                                    .retail_range_end = retail.range_end,
                                    .beta_same_address_range_end = beta_same_address.range_end,
                                    .retail_instructions = retail.instructions.size(),
                                    .retail_basic_blocks = retail.basic_blocks.size(),
                                    .retail_normalized_opcode_signature = retail_profile.instruction_signatures,
                                    .beta_same_address_normalized_opcode_signature = beta_same_profile.instruction_signatures,
                                    .same_address_match = classify(retail, beta_same_address),
                                    .same_address_changed_blocks = changed_blocks(retail, beta_same_address),
                                    .unmatched_retail_instructions = mismatching_instructions(retail, beta_same_address),
                                    .unmatched_beta_instructions = mismatching_instructions(beta_same_address, retail)};
        std::vector<AnalogCandidate> candidates;
        std::size_t scanned = 0;
        for (std::uint32_t candidate = 0; candidate + target_budget(target) <= beta_rom.size(); candidate += 2U) {
            if (!candidate_entry_prefilter(beta_rom, candidate, retail.instructions.front()) ||
                !has_relative_anchor(beta_rom, candidate, retail)) continue;
            if (++scanned > kMaxAnalogCandidatesScanned) break;
            const auto beta_candidate = decode_target(beta_rom, target, candidate);
            auto analog = make_candidate(candidate, retail, beta_candidate);
            if (analog.match != MatchKind::unmatched) candidates.push_back(std::move(analog));
        }
        std::sort(candidates.begin(), candidates.end(), [](const auto& left, const auto& right) {
            return std::tuple{match_rank(left.match), left.matching_instructions, left.matching_blocks,
                               std::numeric_limits<std::uint32_t>::max() - left.beta_entry} >
                   std::tuple{match_rank(right.match), right.matching_instructions, right.matching_blocks,
                              std::numeric_limits<std::uint32_t>::max() - right.beta_entry};
        });
        if (candidates.size() > kMaxReportedAnalogs) candidates.resize(kMaxReportedAnalogs);
        comparison.analogs = std::move(candidates);
        report.targets.push_back(std::move(comparison));
    }
    return report;
}

} // namespace oasis::tools
