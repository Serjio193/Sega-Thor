#include "core/rom.hpp"
#include "tools/re_candidate_map.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {

std::string read_file(const char* path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("unable to open Ghidra map: " + std::string(path));
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void write_file(const char* path, const std::string& content) {
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("unable to open candidate output: " + std::string(path));
    output << content;
    if (!output) throw std::runtime_error("unable to write candidate output: " + std::string(path));
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 5 && argc != 6) {
        std::cerr << "usage: oasis_re_candidate_map <retail_rom> <ghidra.json> <map.json> <top.txt> [beta_rom]\n";
        return 2;
    }
    try {
        const auto retail = oasis::Rom::load(argv[1]);
        const auto ghidra = oasis::tools::parse_ghidra_map(read_file(argv[2]));
        std::optional<oasis::Rom> beta_storage;
        std::optional<std::span<const std::uint8_t>> beta;
        if (argc == 6) {
            beta_storage = oasis::Rom::load(argv[5]);
            beta = beta_storage->bytes();
        }
        const auto atlas = oasis::tools::build_rom_atlas(retail.bytes(), beta);
        const auto report = oasis::tools::build_candidate_map(ghidra, atlas);
        write_file(argv[3], oasis::tools::candidate_map_to_json(report));
        write_file(argv[4], oasis::tools::candidate_map_to_text(report));
        std::cout << "normalized " << report.candidates.size() << " unique candidate entries\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
