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
