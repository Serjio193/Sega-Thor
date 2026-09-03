#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "tools/re_trace.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void write_text(const char* path, const std::string& content) {
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("unable to open trace output: " + std::string(path));
    output << content;
    if (!output) throw std::runtime_error("unable to write trace output: " + std::string(path));
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: oasis_re_trace <usa_rom> <trace.json> <trace.txt>\n";
        return 2;
    }
    try {
        const auto rom = oasis::Rom::load(argv[1]);
        const auto identity = oasis::identify_rom(rom.bytes());
        if (identity.status != oasis::RomSupportStatus::Supported) {
            throw std::runtime_error("RE trace requires the supported USA reference ROM");
        }
        const auto report = oasis::tools::trace_m68k_scenario(rom.bytes());
        write_text(argv[2], oasis::tools::trace_to_json(report));
        write_text(argv[3], oasis::tools::trace_to_text(report));
        std::cout << "traced " << report.executed_instructions.size()
                  << " bounded instructions\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
