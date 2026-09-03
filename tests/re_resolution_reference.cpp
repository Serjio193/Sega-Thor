#include "core/rom.hpp"
#include "tools/re_atlas.hpp"
#include "tools/re_resolution.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    const auto rom = oasis::Rom::load(argv[1]);
    const auto atlas = oasis::tools::build_rom_atlas(rom.bytes());
    const auto report = oasis::tools::resolve_bounded_displacements(atlas, rom.bytes());
    if (report.target_entry != 0x60004 || report.static_candidate_count == 0 ||
        report.newly_resolved == 0 || report.atlas_unresolved_after != report.atlas_unresolved_before - report.newly_resolved ||
        report.provenance_failures != 0) {
        throw std::runtime_error("bounded USA resolution oracle mismatch");
    }
    if (report.ranking_delta.size() != 3 || report.items.empty())
        throw std::runtime_error("resolution report shape mismatch");
    const auto find_item = [&](std::uint32_t address) {
        return std::find_if(report.items.begin(), report.items.end(),
                            [=](const auto& item) { return item.instruction_address == address; });
    };
    const auto first = find_item(0x60516);
    if (first == report.items.end() || first->block_start != 0x60516 ||
        first->displacement != 1 || first->base_value != 0x00FF001A ||
        first->effective_address != 0x00FF001B || first->provenance.empty() ||
        rom.bytes()[0x60434] != 0x4B || rom.bytes()[0x60435] != 0xF9 ||
        rom.bytes()[0x60438] != 0x00 || rom.bytes()[0x60439] != 0x1A) {
        throw std::runtime_error("resolution byte/provenance oracle mismatch");
    }
    if (oasis::tools::resolution_to_json(report) != oasis::tools::resolution_to_json(report))
        throw std::runtime_error("resolution JSON is not deterministic");
    std::cout << "verified resolution candidates=" << report.static_candidate_count
              << " resolved=" << report.newly_resolved << " atlas="
              << report.atlas_unresolved_before << " -> " << report.atlas_unresolved_after << '\n';
    return 0;
}
