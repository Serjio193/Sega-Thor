#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "tools/re_caller_stack.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {

void write_file(const char* path, const std::string& content) {
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("unable to open caller-stack output: " + std::string(path));
    output << content;
    if (!output) throw std::runtime_error("unable to write caller-stack output: " + std::string(path));
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: oasis_re_caller_stack <usa_rom> <report.json> <report.txt>\n";
        return 2;
    }
    try {
        const auto rom = oasis::Rom::load(argv[1]);
        if (oasis::identify_rom(rom.bytes()).status != oasis::RomSupportStatus::Supported)
            throw std::runtime_error("caller-stack report requires the supported USA reference ROM");
        const auto report = oasis::tools::audit_caller_stack(rom.bytes());
        write_file(argv[2], oasis::tools::caller_stack_to_json(report));
        write_file(argv[3], oasis::tools::caller_stack_to_text(report));
        std::cout << "audited caller stack paths=" << report.relevant_path_count
                  << "; memory[P]=" << report.value_kind
                  << "; unresolved=" << report.reachable_unresolved_after << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
