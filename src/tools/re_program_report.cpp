#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "tools/re_program.hpp"

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

std::span<const oasis::tools::FunctionTarget> representative_targets() {
    static const oasis::tools::FunctionTarget targets[] = {
        {.entry = 0x00003820, .byte_budget = 0, .confirmed_end = 0x00003B3E},
        {.entry = 0x00008E90, .byte_budget = 0x120, .confirmed_end = std::nullopt},
        {.entry = 0x0000A6A4, .byte_budget = 0x180, .confirmed_end = std::nullopt},
        {.entry = 0x0000D3B2, .byte_budget = 0, .confirmed_end = 0x0000D406},
    };
    return targets;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: oasis_re_program <usa_rom> <report.json> <report.txt>\n";
        return 2;
    }
    try {
        const auto rom = oasis::Rom::load(argv[1]);
        const auto identity = oasis::identify_rom(rom.bytes());
        if (identity.status != oasis::RomSupportStatus::Supported) {
            throw std::runtime_error("RE program report requires the supported USA reference ROM");
        }
        const auto report = oasis::tools::analyze_m68k_functions(rom.bytes(), representative_targets());
        write_text(argv[2], oasis::tools::program_to_json(report));
        write_text(argv[3], oasis::tools::program_to_text(report));
        std::cout << "analyzed " << report.functions.size() << " bounded functions\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
