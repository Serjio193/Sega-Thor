#include "tools/re_diff.hpp"

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

namespace {

bool has_analog(const oasis::tools::TargetComparison& target, std::uint32_t entry,
                oasis::tools::MatchKind kind) {
    for (const auto& analog : target.analogs) {
        if (analog.beta_entry == entry && analog.match == kind) return true;
    }
    return false;
}

bool has_classification(const oasis::tools::InstructionDifference& difference,
                        oasis::tools::InstructionDiffKind kind) {
    for (const auto item : difference.classifications) {
        if (item == kind) return true;
    }
    return false;
}

const oasis::tools::AnalogCandidate& find_analog(const oasis::tools::TargetComparison& target,
                                                  std::uint32_t entry) {
    for (const auto& candidate : target.analogs) {
        if (candidate.beta_entry == entry) return candidate;
    }
    throw "missing synthetic analogue";
}

} // namespace

int main() {
    using namespace oasis::tools;
    std::vector<std::uint8_t> retail(0x100, 0x4E);
    std::vector<std::uint8_t> beta(0x120, 0x4E);
    retail[0x00] = 0x70; retail[0x01] = 0x01; retail[0x02] = 0x4E; retail[0x03] = 0x75;
    beta[0x00] = 0x70; beta[0x01] = 0x01; beta[0x02] = 0x4E; beta[0x03] = 0x75;
    retail[0x20] = 0x70; retail[0x21] = 0x01; retail[0x22] = 0x4E; retail[0x23] = 0x75;
    beta[0x40] = 0x70; beta[0x41] = 0x02; beta[0x42] = 0x4E; beta[0x43] = 0x75;
    retail[0x60] = 0x70; retail[0x61] = 0x01; retail[0x62] = 0x4E;
    retail[0x63] = 0x71; retail[0x64] = 0x4E; retail[0x65] = 0x75;
    beta[0x80] = 0x70; beta[0x81] = 0x02; beta[0x82] = 0x70;
    beta[0x83] = 0x03; beta[0x84] = 0x4E; beta[0x85] = 0x75;

    const std::vector<DifferentialTarget> targets{
        {.entry = 0x00, .byte_budget = 0x04, .confirmed_end = 0x04},
        {.entry = 0x20, .byte_budget = 0x04, .confirmed_end = 0x24},
        {.entry = 0x60, .byte_budget = 0x06, .confirmed_end = 0x66},
    };
    const auto report = compare_m68k_revisions(retail, beta, targets);
    assert(report.targets.size() == 3U);
    assert(report.targets[0].same_address_match == MatchKind::exact_match);
    assert(report.targets[1].same_address_match != MatchKind::exact_match);
    assert(has_analog(report.targets[1], 0x40U, MatchKind::structural_match));
    assert(report.targets[1].analogs.front().matching_instructions == 2U);
    assert(has_analog(report.targets[2], 0x80U, MatchKind::changed_blocks));
    const auto& moved = find_analog(report.targets[1], 0x40U);
    assert(moved.changed_block_details.size() == 1U);
    assert(has_classification(moved.changed_block_details.front().instruction_differences[0],
                              InstructionDiffKind::constant_changed));
    const auto& changed = find_analog(report.targets[2], 0x80U).changed_block_details;
    assert(changed.size() == 1U);
    assert(changed.front().instruction_differences.size() == 3U);
    assert(has_classification(changed.front().instruction_differences[0],
                              InstructionDiffKind::constant_changed));
    assert(has_classification(changed.front().instruction_differences[1],
                              InstructionDiffKind::unresolved));

    const auto json = diff_to_json(report);
    assert(json.find("oasis.m68k.re-diff.v1") != std::string::npos);
    assert(json.find("exact_match") != std::string::npos);
    assert(json.find("structural_match") != std::string::npos);
    assert(json.find("changed_blocks") != std::string::npos);
    assert(json == diff_to_json(report));
    const auto text = diff_to_text(report);
    assert(text.find("no semantic conclusions") != std::string::npos);
    return 0;
}
