#include "tools/re_atlas.hpp"

#include <cassert>
#include <string>

int main() {
    using namespace oasis::tools;
    AtlasReport report;
    report.entries = {
        {.id = "rom_00000100", .type = AtlasEntryType::function, .start = 0x100,
         .end = 0x110, .bounded_evidence_end = 0x110, .callers = {0x200}, .callees = {0x300},
         .direct_rom_refs = {0x500}, .direct_ram_refs = {0xFF1000}},
        {.id = "rom_00000200", .type = AtlasEntryType::bounded_code, .start = 0x200,
         .bounded_evidence_end = 0x220, .unresolved_reference_count = 1},
        {.id = "table_00000500", .type = AtlasEntryType::table, .start = 0x500, .end = 0x508,
         .boundary_confidence = AtlasConfidence::confirmed},
    };
    report.call_edges = {{0x100, 0x300, {0x10A}}};
    assert(atlas_entries_at(report, 0x105).size() == 1);
    assert(atlas_callers(report, 0x100).at(0) == 0x200);
    assert(atlas_callees(report, 0x100).at(0) == 0x300);
    assert(atlas_refs_to_rom(report, 0x500).at(0)->start == 0x100);
    assert(atlas_refs_to_ram(report, 0xFF1000).at(0)->start == 0x100);
    assert(atlas_entries_with_unresolved(report).size() == 1);
    assert(atlas_entries_without_native(report).size() == 3);
    report.entries.push_back({.id = "data_00000108", .type = AtlasEntryType::data, .start = 0x108, .end = 0x118});
    report.conflicts = detect_atlas_conflicts(report.entries);
    assert(report.conflicts.size() == 1);
    report.retail.id = "synthetic";
    report.retail.display_name = "synthetic";
    report.coverage.atlas_entries = report.entries.size();
    const auto json = atlas_to_json(report);
    assert(json.find("oasis.m68k.re-atlas.v1") != std::string::npos);
    assert(json.find("incompatible typed evidence overlap") != std::string::npos);
    assert(json.find("\"start\":\"0x00000100\"") != std::string::npos);
    assert(json.find("\"callers\":[\"0x00000200\"]") != std::string::npos);
    assert(atlas_beta_lookup(report, 0x100) == std::nullopt);
    return 0;
}
