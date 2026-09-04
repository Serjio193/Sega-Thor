#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "tools/re_atlas.hpp"
#include "tools/re_reachable_closure.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {

void write_file(const char* path, const std::string& content) {
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("unable to open closure output: " + std::string(path));
    output << content;
    if (!output) throw std::runtime_error("unable to write closure output: " + std::string(path));
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: oasis_re_reachable_closure <usa_rom> <report.json> <report.txt>\n";
        return 2;
    }
    try {
        const auto rom = oasis::Rom::load(argv[1]);
        if (oasis::identify_rom(rom.bytes()).status != oasis::RomSupportStatus::Supported)
            throw std::runtime_error("closure report requires the supported USA reference ROM");
        const auto atlas = oasis::tools::build_rom_atlas(rom.bytes());
        const auto report = oasis::tools::audit_reachable_unresolved(atlas, rom.bytes());
        write_file(argv[2], oasis::tools::reachable_closure_to_json(report));
        write_file(argv[3], oasis::tools::reachable_closure_to_text(report));
        std::cout << "audited " << report.exact_reachable_unresolved_count
                  << " reachable unresolved refs; newly resolved=" << report.newly_resolved << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
