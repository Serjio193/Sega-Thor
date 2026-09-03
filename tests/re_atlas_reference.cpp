#include "core/rom.hpp"
#include "tools/re_atlas.hpp"
#include "tools/re_atlas_ranking.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    if (argc != 3) return 2;
    const auto retail = oasis::Rom::load(argv[1]);
    const auto beta = oasis::Rom::load(argv[2]);
    const auto report = oasis::tools::build_rom_atlas(retail.bytes(), beta.bytes());
    const auto has = [&](std::uint32_t address) {
        return std::find_if(report.entries.begin(), report.entries.end(),
                            [=](const auto& entry) { return entry.start == address; }) != report.entries.end();
    };
    if (report.entries.size() != 13 || !has(0x3820) || !has(0x60004) || !has(0x82AE) ||
        !has(0x7A28) || !has(0xA6A4) || !has(0x5CE96) || report.conflicts.size() != 0) {
        throw std::runtime_error("atlas manifest/oracle entry set mismatch");
    }
    const auto find_entry = [&](std::uint32_t address) {
        return std::find_if(report.entries.begin(), report.entries.end(),
                            [=](const auto& entry) { return entry.start == address; });
    };
    if (find_entry(0x3820)->end != 0x3B3E || find_entry(0xD3B2)->end != 0xD406 ||
        oasis::tools::atlas_callees(report, 0xD3B2).at(0) != 0x3820 ||
        oasis::tools::atlas_refs_to_rom(report, 0x5CE96).empty() ||
        oasis::tools::atlas_refs_to_ram(report, 0xFF2FA8).empty()) {
        throw std::runtime_error("atlas raw boundary/call/reference mismatch");
    }
    const auto beta_match = oasis::tools::atlas_beta_lookup(report, 0xA6A4);
    if (!beta_match || beta_match->address != 0xA654 ||
        beta_match->match != oasis::tools::AtlasCorrespondence::structural) {
        throw std::runtime_error("A6A4 beta correspondence mismatch");
    }
    if (beta_match->changed_blocks.size() != 1 || beta_match->changed_blocks[0] != 10) {
        throw std::runtime_error("A6A4 changed block evidence mismatch");
    }
    const auto dynamic = std::find_if(report.entries.begin(), report.entries.end(),
                                      [](const auto& entry) { return entry.start == 0xA6A4; });
    if (dynamic == report.entries.end() || !dynamic->dynamic || dynamic->dynamic->executed_instructions != 5 ||
        dynamic->dynamic->memory_reads != 2 || dynamic->dynamic->raw_facts.size() != 3) {
        throw std::runtime_error("A6A4 dynamic evidence mismatch");
    }
    if (report.coverage.confirmed_classified_bytes != (0x3B3E - 0x3820) + (0xD406 - 0xD3B2) + (0x5D046 - 0x5CE96)) {
        throw std::runtime_error("atlas coverage mismatch");
    }
    const auto ranking = oasis::tools::rank_atlas_unresolved(report);
    if (ranking.atlas_unresolved_reference_count != 577 ||
        ranking.dynamic_resolvable_candidate_count != 2 ||
        ranking.constant_propagation_candidate_count != 168 ||
        ranking.unsupported_decoder_item_count != 4) {
        throw std::runtime_error("atlas ranking totals mismatch");
    }
    const auto has_group = [&](const char* dimension, const char* key, std::size_t potential) {
        return std::find_if(ranking.groups.begin(), ranking.groups.end(), [&](const auto& group) {
            return group.dimension == dimension && group.key == key && group.potentially_resolvable_refs == potential;
        }) != ranking.groups.end();
    };
    if (!has_group("addressing_mode", "address_displacement", 446) ||
        !has_group("containing_function", "0x00060004", 424) ||
        !has_group("register", "A6", 387)) {
        throw std::runtime_error("atlas ranking priority mismatch");
    }
    const auto first = oasis::tools::atlas_to_json(report);
    if (first != oasis::tools::atlas_to_json(report)) throw std::runtime_error("atlas JSON is not deterministic");
    std::cout << "verified atlas entries=" << report.entries.size()
              << " conflicts=" << report.conflicts.size() << '\n';
    return 0;
}
