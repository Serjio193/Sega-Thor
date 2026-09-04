#include "core/rom.hpp"
#include "tools/re_atlas.hpp"
#include "tools/re_cfg_audit.hpp"

#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    const auto rom = oasis::Rom::load(argv[1]);
    const auto atlas = oasis::tools::build_rom_atlas(rom.bytes());
    const auto report = oasis::tools::audit_bounded_unreachable_cfg(atlas, rom.bytes());
    if (report.raw_static_evidence_records != 80 || report.outside_reachable_records != 80 ||
        report.nonreachable_unresolved != 80 || report.raw_unresolved_after_resolution != 96 ||
        report.reachable_unresolved_after_resolution != 16 || report.records.size() != 80) {
        throw std::runtime_error("USA CFG audit accounting mismatch");
    }
    if (report.records_without_known_incoming_edges + report.records_with_known_incoming_edges != 80 ||
        report.classification_counts.empty() || report.islands.empty()) {
        throw std::runtime_error("USA CFG audit structure mismatch");
    }
    const auto& first = report.records.front();
    if (first.instruction_address == 0 || first.byte_end <= first.instruction_address ||
        !first.nearest_following_reachable) {
        throw std::runtime_error("USA CFG audit record evidence mismatch");
    }
    if (oasis::tools::cfg_audit_to_json(report) != oasis::tools::cfg_audit_to_json(report))
        throw std::runtime_error("USA CFG audit JSON is not deterministic");
    std::cout << "verified CFG audit records=" << report.records.size() << " islands=" << report.islands.size()
              << " incoming=" << report.records_with_known_incoming_edges << '\n';
    return 0;
}
