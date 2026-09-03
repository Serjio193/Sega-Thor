#include "core/rom.hpp"
#include "tools/re_diff.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void write_text(const char* path, const std::string& content) {
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("unable to open report output: " + std::string(path));
    output << content;
    if (!output) throw std::runtime_error("unable to write report output: " + std::string(path));
}

std::span<const oasis::tools::DifferentialTarget> targets() {
    static const oasis::tools::DifferentialTarget values[] = {
        {.entry = 0x00003820, .byte_budget = 0, .confirmed_end = 0x00003B3E},
        {.entry = 0x00060004, .byte_budget = 0x1200, .confirmed_end = std::nullopt},
        {.entry = 0x000082AE, .byte_budget = 0x180, .confirmed_end = std::nullopt},
        {.entry = 0x00007A28, .byte_budget = 0x180, .confirmed_end = std::nullopt},
        {.entry = 0x0000A6A4, .byte_budget = 0x180, .confirmed_end = std::nullopt},
    };
    return values;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "usage: oasis_re_diff <retail_rom> <beta_rom> <report.json> <report.txt>\n";
        return 2;
    }
    try {
        const auto retail = oasis::Rom::load(argv[1]);
        const auto beta = oasis::Rom::load(argv[2]);
        const auto report = oasis::tools::compare_m68k_revisions(retail.bytes(), beta.bytes(), targets());
        write_text(argv[3], oasis::tools::diff_to_json(report));
        write_text(argv[4], oasis::tools::diff_to_text(report));
        std::cout << "compared " << report.targets.size() << " bounded targets\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
