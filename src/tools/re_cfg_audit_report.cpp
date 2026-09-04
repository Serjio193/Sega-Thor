#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "tools/re_atlas.hpp"
#include "tools/re_cfg_audit.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {

void write_file(const char* path, const std::string& content) {
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("unable to open CFG audit output: " + std::string(path));
    output << content;
    if (!output) throw std::runtime_error("unable to write CFG audit output: " + std::string(path));
}

}

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: oasis_re_cfg_audit <usa_rom> <report.json> <report.txt>\n";
        return 2;
    }
    try {
        const auto rom = oasis::Rom::load(argv[1]);
        if (oasis::identify_rom(rom.bytes()).status != oasis::RomSupportStatus::Supported)
            throw std::runtime_error("CFG audit requires the supported USA reference ROM");
        const auto atlas = oasis::tools::build_rom_atlas(rom.bytes());
        const auto report = oasis::tools::audit_bounded_unreachable_cfg(atlas, rom.bytes());
        write_file(argv[2], oasis::tools::cfg_audit_to_json(report));
        write_file(argv[3], oasis::tools::cfg_audit_to_text(report));
        std::cout << "audited " << report.outside_reachable_records << " outside-reachable records in "
                  << report.islands.size() << " islands\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
