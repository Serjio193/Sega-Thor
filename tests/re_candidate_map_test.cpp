#include "tools/re_candidate_map.hpp"

#include <cassert>
#include <stdexcept>
#include <string>

using namespace oasis::tools;

namespace {

GhidraMap synthetic_ghidra() {
    return parse_ghidra_map(R"json({
      "schema":"oasis.m68k.ghidra-map.v1",
      "metadata":{
        "program_name":"synthetic.bin",
        "language_id":"68000:BE:32:default",
        "compiler_spec":"default",
        "analysis_mode":"test",
        "semantic_status":"GHIDRA_CANDIDATE_ONLY"
      },
      "functions":[
        {"entry":"0x003820","range":"0x003820..0x003830","source":"GHIDRA_RECOGNIZED_FUNCTION",
         "called_by":[],"calls":[],"has_return":true,"instruction_count":2,"basic_block_count":1},
        {"entry":"0x000150","range":"0x000150..0x000160","source":"GHIDRA_RECOGNIZED_FUNCTION",
         "called_by":[],"calls":[],"has_return":true,"instruction_count":2,"basic_block_count":1},
        {"entry":"0x000160","range":"0x000160..0x000170","source":"GHIDRA_RECOGNIZED_FUNCTION",
         "called_by":[],"calls":[],"has_return":true,"instruction_count":2,"basic_block_count":1},
        {"entry":"0x000180","range":"0x000180..0x0001A0","source":"GHIDRA_RECOGNIZED_FUNCTION",
         "called_by":[],"calls":[],"has_return":true,"instruction_count":2,"basic_block_count":1},
        {"entry":"0x000300","range":"0x000300..0x000304","source":"GHIDRA_RECOGNIZED_FUNCTION",
         "called_by":["0x000250"],"calls":[],"has_return":true,"instruction_count":1,"basic_block_count":1}
      ],
      "candidates":[
        {"entry":"0x003820","range":"0x003820..0x003830","source":"GHIDRA_RECOGNIZED_FUNCTION",
         "decoded_as_code":true},
        {"entry":"0x000300","range":"0x000300..0x000304","source":"VECTOR_OR_DIRECT_CALL_TARGET",
         "decoded_as_code":true}
      ]
    })json");
}

AtlasReport synthetic_atlas() {
    AtlasReport report;
    AtlasEntry confirmed{.id = "rom_00003820", .type = AtlasEntryType::function,
                         .start = 0x3820, .end = 0x3B3E,
                         .semantic_confidence = AtlasConfidence::confirmed,
                         .boundary_confidence = AtlasConfidence::confirmed,
                         .verification_status = "VERIFIED"};
    AtlasEntry table{.id = "table_00000190", .type = AtlasEntryType::table,
                     .start = 0x190, .end = 0x1A0,
                     .boundary_confidence = AtlasConfidence::confirmed};
    AtlasEntry static_target{.id = "rom_00000300", .type = AtlasEntryType::bounded_code,
                             .start = 0x300, .bounded_evidence_end = 0x304};
    static_target.beta = AtlasBetaCorrespondence{0x2F0, AtlasCorrespondence::exact, {}};
    report.entries = {confirmed, table, static_target};
    report.call_edges = {{0x250, 0x300, {0x250}}};
    return report;
}

void test_parser_and_duplicate_merge() {
    const auto parsed = synthetic_ghidra();
    assert(parsed.functions.size() == 5U && parsed.candidates.size() == 2U);
    const auto report = build_candidate_map(parsed, synthetic_atlas());
    const auto find = [&](std::uint32_t address) {
        for (const auto& item : report.candidates) if (item.entry == address) return &item;
        return static_cast<const CandidateRecord*>(nullptr);
    };
    const auto* confirmed = find(0x3820);
    assert(confirmed && confirmed->ghidra_function);
    assert(confirmed->classification == CandidateClassification::confirmed);
    assert(confirmed->boundary_conflict && confirmed->source_flags.size() >= 2U);
    const auto* static_target = find(0x300);
    assert(static_target && static_target->classification == CandidateClassification::static_supported);
    assert(static_target->beta_match_kind == "exact");
    const auto* conflict = find(0x180);
    assert(conflict && conflict->classification == CandidateClassification::conflict);
    assert(conflict->code_data_conflict);
}

void test_dynamic_and_tie_breaking() {
    const auto report = build_candidate_map(synthetic_ghidra(), AtlasReport{});
    const auto find = [&](std::uint32_t address) {
        for (const auto& item : report.candidates) if (item.entry == address) return &item;
        return static_cast<const CandidateRecord*>(nullptr);
    };
    const auto* dynamic = find(0x611EE);
    assert(dynamic && dynamic->dynamic_observed);
    const auto* first = find(0x150);
    const auto* second = find(0x160);
    assert(first && second && first->classification == CandidateClassification::ghidra_only);
    assert(first->ranking_score == second->ranking_score);
    std::size_t first_index{};
    for (; first_index < report.candidates.size(); ++first_index)
        if (report.candidates[first_index].entry == 0x150) break;
    std::size_t second_index{};
    for (; second_index < report.candidates.size(); ++second_index)
        if (report.candidates[second_index].entry == 0x160) break;
    assert(first_index < second_index);
}

void test_malformed_and_deterministic_output() {
    bool rejected = false;
    try { (void)parse_ghidra_map("{} trailing"); } catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
    const auto report = build_candidate_map(synthetic_ghidra(), synthetic_atlas());
    const auto json = candidate_map_to_json(report);
    assert(json == candidate_map_to_json(report));
    assert(json.find("oasis.m68k.re-candidate-map.v1") != std::string::npos);
    const auto text = candidate_map_to_text(report, 20);
    assert(text == candidate_map_to_text(report, 20));
    assert(text.find("top_10_non_confirmed_quality_audit") != std::string::npos);
}

} // namespace

int main() {
    test_parser_and_duplicate_merge();
    test_dynamic_and_tie_breaking();
    test_malformed_and_deterministic_output();
    return 0;
}
