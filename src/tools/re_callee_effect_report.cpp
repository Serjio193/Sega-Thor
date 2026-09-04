#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "tools/re_callee_effect.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {

void write_file(const char* path, const std::string& content) {
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("unable to open callee-effect output: " + std::string(path));
    output << content;
    if (!output) throw std::runtime_error("unable to write callee-effect output: " + std::string(path));
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: oasis_re_callee_effect <usa_rom> <report.json> <report.txt>\n";
        return 2;
    }
    try {
        const auto rom = oasis::Rom::load(argv[1]);
        if (oasis::identify_rom(rom.bytes()).status != oasis::RomSupportStatus::Supported)
            throw std::runtime_error("callee-effect report requires the supported USA reference ROM");
        const auto report = oasis::tools::audit_callee_effect(rom.bytes());
        write_file(argv[2], oasis::tools::callee_effect_to_json(report));
        write_file(argv[3], oasis::tools::callee_effect_to_text(report));
        std::cout << "audited callee " << std::hex << "0x" << report.entry
                  << "; returns=" << std::dec << report.return_sites.size()
                  << "; stack=" << report.stack_effect.status << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
