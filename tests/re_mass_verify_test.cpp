#include "tools/re_mass_verify.hpp"

#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

using namespace oasis::tools;

namespace {

const MassCandidate* find(const MassVerificationReport& report, std::uint32_t address) {
    for (const auto& item : report.candidates) if (item.entry == address) return &item;
    return nullptr;
}

CandidateMapReport candidates() {
    CandidateMapReport report;
    report.schema = "oasis.m68k.re-candidate-map.v1";
    report.ghidra_schema = "oasis.m68k.ghidra-map.v1";
    report.ghidra_program = "synthetic.bin";
    const auto make = [](std::uint32_t entry, std::uint32_t end, CandidateClassification classification) {
        CandidateRecord record;
        record.entry = entry;
        record.source_flags = {"GHIDRA_FUNCTION"};
        record.ghidra_function = true;
        record.ghidra_range_start = entry;
        record.ghidra_range_end = end;
        record.complexity = CandidateComplexity::leaf;
        record.classification = classification;
        return record;
    };
    auto first = make(0x100, 0x102, CandidateClassification::ghidra_only);
    first.source_flags.push_back("GHIDRA_DIRECT_BSR_TARGET");
    first.known_direct_call_target = true;
    first.ghidra_called_by = {0x200};
    report.candidates.push_back(first);
    report.candidates.push_back(make(0x110, 0x112, CandidateClassification::ghidra_only));
    report.candidates.push_back(make(0x120, 0x130, CandidateClassification::ghidra_only));
    report.candidates.push_back(make(0x3820, 0x3822, CandidateClassification::confirmed));
    return report;
}

AtlasReport atlas() {
    AtlasReport report;
    report.entries.push_back({"table", AtlasEntryType::table, 0x124, 0x128, {}, AtlasConfidence::unknown,
                              AtlasConfidence::confirmed});
    return report;
}

void test_categories_and_leaf_breakdown() {
    std::vector<std::uint8_t> rom(0x4000, 0);
    rom[0x100] = 0x4E; rom[0x101] = 0x75;
    rom[0x110] = 0xF4; rom[0x111] = 0x00;
    rom[0x120] = 0x4E; rom[0x121] = 0x75;
    rom[0x3820] = 0x4E; rom[0x3821] = 0x75;
    const auto report = verify_mass_structure(rom, candidates(), atlas());
    assert(report.total_candidates == 4U);
    assert(find(report, 0x100)->structural_classification == StructuralClassification::strong_static);
    assert(find(report, 0x110)->structural_classification == StructuralClassification::unsupported);
    assert(find(report, 0x120)->structural_classification == StructuralClassification::data_conflict);
    assert(find(report, 0x3820)->previous_classification == CandidateClassification::confirmed);
    assert(find(report, 0x3820)->structural_classification != StructuralClassification::decode_failure);
    assert(report.leaf.total == 4U && report.leaf.clean == 2U && report.leaf.unsupported == 1U);
    assert(report.leaf.boundary_conflict == 1U);
    assert(!report.failure_clusters.empty() && report.failure_clusters.front().count >= 1U);
}

void test_determinism_and_validation() {
    std::vector<std::uint8_t> rom(0x4000, 0x4E);
    rom[0x101] = 0x75; rom[0x111] = 0x75; rom[0x121] = 0x75; rom[0x3820] = 0x4E; rom[0x3821] = 0x75;
    const auto report = verify_mass_structure(rom, candidates(), atlas());
    const auto json = mass_verify_to_json(report);
    const auto text = mass_verify_to_text(report);
    assert(json == mass_verify_to_json(report));
    assert(text == mass_verify_to_text(report));
    assert(json.find("ghidra_only_structural_counts") != std::string::npos);
    assert(text.find("failure_clusters:") != std::string::npos);
    assert(structural_classification_name(StructuralClassification::indirect_flow) == "INDIRECT_FLOW");
    assert(mass_boundary_name(MassBoundaryStatus::shorter) == "BOUNDARY_SHORTER");
    auto duplicate = candidates();
    duplicate.candidates.push_back(duplicate.candidates.front());
    bool rejected = false;
    try { (void)verify_mass_structure(rom, duplicate, atlas()); } catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
}

} // namespace

int main() {
    test_categories_and_leaf_breakdown();
    test_determinism_and_validation();
    return 0;
}
