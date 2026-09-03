#include "core/rom.hpp"
#include "tools/re_diff.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

const oasis::tools::TargetComparison& target(const oasis::tools::DifferentialReport& report,
                                             std::uint32_t entry) {
    for (const auto& item : report.targets) {
        if (item.target.entry == entry) return item;
    }
    throw std::runtime_error("target missing from differential report");
}

const oasis::tools::AnalogCandidate& analog(const oasis::tools::TargetComparison& item,
                                             std::uint32_t entry) {
    for (const auto& candidate : item.analogs) {
        if (candidate.beta_entry == entry) return candidate;
    }
    throw std::runtime_error("expected beta analogue missing");
}

bool has_analog(const oasis::tools::TargetComparison& item, std::uint32_t entry,
                oasis::tools::MatchKind kind) {
    for (const auto& analog : item.analogs) {
        if (analog.beta_entry == entry && analog.match == kind) return true;
    }
    return false;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: oasis_re_diff_reference <retail_rom> <beta_rom>\n";
        return 2;
    }
    try {
        const auto retail = oasis::Rom::load(argv[1]);
        const auto beta = oasis::Rom::load(argv[2]);
        const auto retail_identity = oasis::identify_rom(retail.bytes());
        const auto beta_identity = oasis::identify_rom(beta.bytes());
        if (retail_identity.fingerprint.crc32 != 0xC4728225U ||
            beta_identity.fingerprint.crc32 != 0xFA59F847U ||
            retail.size() != 3145728U || beta.size() != 3145728U ||
            !beta_identity.fingerprint.sega_checksum_valid) {
            throw std::runtime_error("retail/beta fingerprint oracle mismatch");
        }
        const std::vector<oasis::tools::DifferentialTarget> targets{
            {.entry = 0x3820, .byte_budget = 0, .confirmed_end = 0x3B3E},
            {.entry = 0x60004, .byte_budget = 0x1200, .confirmed_end = std::nullopt},
            {.entry = 0x82AE, .byte_budget = 0x180, .confirmed_end = std::nullopt},
            {.entry = 0x7A28, .byte_budget = 0x180, .confirmed_end = std::nullopt},
            {.entry = 0xA6A4, .byte_budget = 0x180, .confirmed_end = std::nullopt},
        };
        const auto report = oasis::tools::compare_m68k_revisions(retail.bytes(), beta.bytes(), targets);
        if (target(report, 0x60004).same_address_match != oasis::tools::MatchKind::exact_match) {
            throw std::runtime_error("60004 exact match was not reproduced");
        }
        if (!has_analog(target(report, 0x3820), 0x37D0, oasis::tools::MatchKind::structural_match) &&
            !has_analog(target(report, 0x3820), 0x37D0, oasis::tools::MatchKind::exact_match)) {
            throw std::runtime_error("3820 beta analogue was not reproduced");
        }
        if (!has_analog(target(report, 0x82AE), 0x825E, oasis::tools::MatchKind::structural_match) &&
            !has_analog(target(report, 0x82AE), 0x825E, oasis::tools::MatchKind::exact_match)) {
            throw std::runtime_error("82AE beta analogue was not reproduced");
        }
        const auto& block = analog(target(report, 0xA6A4), 0xA654);
        if (block.match != oasis::tools::MatchKind::structural_match ||
            block.changed_blocks.size() != 1U || block.changed_blocks.front().ordinal != 10U ||
            !block.changed_blocks.front().retail_start || *block.changed_blocks.front().retail_start != 0xA786U ||
            !block.changed_blocks.front().beta_start || *block.changed_blocks.front().beta_start != 0xA736U ||
            block.changed_block_details.size() != 1U) {
            throw std::runtime_error("changed block 10 correspondence was not reproduced");
        }
        const auto& detail = block.changed_block_details.front();
        if (!detail.retail_block || detail.retail_block->start != 0xA786U ||
            detail.retail_block->end != 0xA792U || !detail.beta_block ||
            detail.beta_block->start != 0xA736U || detail.beta_block->end != 0xA742U ||
            detail.retail_predecessors.size() != 1U || detail.beta_predecessors.size() != 1U ||
            detail.retail_successors.size() != 1U || detail.beta_successors.size() != 1U ||
            detail.retail_predecessors.front().source != 0xA6BAU ||
            detail.retail_predecessors.front().target != 0xA786U ||
            detail.beta_predecessors.front().source != 0xA66AU ||
            detail.beta_predecessors.front().target != 0xA736U ||
            detail.retail_successors.front().source != 0xA78EU ||
            detail.retail_successors.front().target != 0xA7D4U ||
            detail.beta_successors.front().source != 0xA73EU ||
            detail.beta_successors.front().target != 0xA784U ||
            detail.retail_fallthrough_edges.size() != 1U ||
            detail.beta_fallthrough_edges.size() != 1U ||
            detail.retail_fallthrough_edges.front().target != 0xA792U ||
            detail.beta_fallthrough_edges.front().target != 0xA742U ||
            !detail.topology_differences.empty() || detail.instruction_differences.size() != 3U) {
            throw std::runtime_error("changed block 10 CFG context mismatch");
        }
        const auto& first_difference = detail.instruction_differences.front();
        if (first_difference.classifications.size() != 1U ||
            first_difference.classifications.front() != oasis::tools::InstructionDiffKind::relocation_only ||
            !first_difference.retail_instruction || !first_difference.beta_instruction ||
            first_difference.retail_instruction->immediate_constants.front().value != 0xA6BEU ||
            first_difference.beta_instruction->immediate_constants.front().value != 0xA66EU ||
            !detail.instruction_differences.back().retail_instruction->branch_condition_code ||
            *detail.instruction_differences.back().retail_instruction->branch_condition_code != 0x0BU ||
            !detail.instruction_differences.back().beta_instruction->branch_condition_code ||
            *detail.instruction_differences.back().beta_instruction->branch_condition_code != 0x0BU) {
            throw std::runtime_error("changed block 10 relocation evidence mismatch");
        }
        const auto json = oasis::tools::diff_to_json(report);
        if (json.find("5111d21c8344cce00765b32b971849f62950d31869307cc479f5ee7febf87a80") == std::string::npos) {
            throw std::runtime_error("beta SHA-256 is missing from report");
        }
        std::cout << "verified retail/beta fingerprints and requested routine correspondences\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
