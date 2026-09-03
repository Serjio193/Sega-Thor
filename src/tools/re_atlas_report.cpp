#include "core/rom.hpp"
#include "tools/re_atlas.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {

void write_file(const char* path, const std::string& content) {
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("unable to open atlas output: " + std::string(path));
    output << content;
    if (!output) throw std::runtime_error("unable to write atlas output: " + std::string(path));
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 4 && argc != 5) {
        std::cerr << "usage: oasis_re_atlas <retail_rom> <atlas.json> <atlas.txt> [beta_rom]\n";
        return 2;
    }
    try {
        const auto retail = oasis::Rom::load(argv[1]);
        std::optional<std::span<const std::uint8_t>> beta;
        std::optional<oasis::Rom> beta_storage;
        if (argc == 5) {
            beta_storage = oasis::Rom::load(argv[4]);
            beta = beta_storage->bytes();
        }
        const auto report = oasis::tools::build_rom_atlas(retail.bytes(), beta);
        write_file(argv[2], oasis::tools::atlas_to_json(report));
        write_file(argv[3], oasis::tools::atlas_to_text(report));
        std::cout << "mapped " << report.entries.size() << " bounded evidence entries\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
