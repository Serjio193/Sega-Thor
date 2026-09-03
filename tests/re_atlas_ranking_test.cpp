#include "tools/re_atlas_ranking.hpp"

#include <cassert>
#include <string>

int main() {
    using namespace oasis::tools;
    AtlasReport atlas;
    atlas.entries.push_back({
        .id = "rom_00006000", .start = 0x6000,
        .unresolved_references = {
            {0x6004, 0x6000, 5, 2, "address_displacement", "move_address", "register_based", true, true},
            {0x6008, 0x6000, 5, 2, "address_displacement", "move_address", "register_based", false, true},
            {0x600C, 0x6000, 6, 3, "address_indexed", "arithmetic", "register_based", false, false},
        },
        .unsupported_evidence = {{0x6010, 0x6000, "opcode", "unsupported_opcode"}},
    });
    const auto report = rank_atlas_unresolved(atlas);
    assert(report.atlas_unresolved_reference_count == 3);
    assert(report.dynamic_resolvable_candidate_count == 1);
    assert(report.constant_propagation_candidate_count == 2);
    assert(report.unsupported_decoder_item_count == 1);
    assert(!report.groups.empty());
    assert(report.groups.front().potentially_resolvable_refs == 3);
    const auto json = ranking_to_json(report);
    const auto text = ranking_to_text(report);
    assert(json.find("oasis.m68k.re-ranking.v1") != std::string::npos);
    assert(json.find("address_displacement") != std::string::npos);
    assert(text.find("If support addressing_mode:address_displacement") != std::string::npos);
    assert(text.find("potentially resolve 2 refs") != std::string::npos);
    return 0;
}
