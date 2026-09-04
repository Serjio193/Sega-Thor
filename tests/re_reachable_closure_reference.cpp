#include "core/rom.hpp"
#include "tools/re_atlas.hpp"
#include "tools/re_reachable_closure.hpp"

#include <algorithm>
#include <iostream>
#include <set>
#include <stdexcept>

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    const auto rom = oasis::Rom::load(argv[1]);
    const auto atlas = oasis::tools::build_rom_atlas(rom.bytes());
    const auto report = oasis::tools::audit_reachable_unresolved(atlas, rom.bytes());
    if (report.exact_reachable_unresolved_count != 16 || report.reachable_unresolved_before != 16 ||
        report.newly_resolved != 0 || report.reachable_unresolved_after != 16 ||
        report.nonreachable_unresolved != 80 || report.speculative_resolutions != 0 ||
        report.provenance_failures != 0 || report.items.size() != 16) {
        throw std::runtime_error("reachable closure accounting mismatch");
    }
    const auto count = [&](const char* key) {
        const auto found = std::find_if(report.reason_counts.begin(), report.reason_counts.end(),
                                        [=](const auto& item) { return item.key == key; });
        return found == report.reason_counts.end() ? 0U : found->count;
    };
    if (count("call_clobber") != 14 || count("unsupported_transfer") != 2 ||
        report.reason_counts.size() != 2 ||
        report.raw_static_unresolved != 577 || report.raw_displacement_backlog != 446 ||
        report.atlas_unresolved_after != 577 || report.ranking_displacement_after != 446) {
        throw std::runtime_error("reachable closure evidence mismatch");
    }
    const std::set<std::uint32_t> expected{
        0x604EA, 0x60BD8, 0x60BFA, 0x60C08, 0x60C1E, 0x60C34, 0x60C4A, 0x60C60,
        0x60C76, 0x60C8A, 0x60C94, 0x60CAA, 0x60CC2, 0x60D94, 0x60DB0, 0x60DC8};
    std::set<std::uint32_t> actual;
    for (const auto& item : report.items) {
        actual.insert(item.instruction_address);
        if (item.opcode == 0 || item.operand.empty() || item.cfg_predecessors.empty() ||
            item.current_unresolved_reason.empty() || item.evidence != "static_bounded_backward_slice") {
            throw std::runtime_error("reachable closure per-item evidence mismatch");
        }
    }
    if (actual != expected) throw std::runtime_error("reachable closure target set mismatch");
    const auto unsupported = std::find_if(report.items.begin(), report.items.end(), [](const auto& item) {
        return item.instruction_address == 0x60BFA;
    });
    if (unsupported == report.items.end() || unsupported->last_known_definitions.empty() ||
        unsupported->last_known_definitions.front().supported) {
        throw std::runtime_error("reachable closure definition evidence mismatch");
    }
    if (rom.bytes()[0x60BD0] != 0x20 || rom.bytes()[0x60BD1] != 0x5F)
        throw std::runtime_error("reachable closure unsupported transfer bytes mismatch");
    const auto json = oasis::tools::reachable_closure_to_json(report);
    if (json != oasis::tools::reachable_closure_to_json(report) ||
        json.find("oasis.m68k.re-reachable-closure.v1") == std::string::npos) {
        throw std::runtime_error("reachable closure report mismatch");
    }
    std::cout << "verified reachable closure refs=" << report.items.size()
              << " newly_resolved=" << report.newly_resolved << '\n';
    return 0;
}
