#include "core/rom.hpp"
#include "tools/re_atlas.hpp"
#include "tools/re_atlas_ranking.hpp"

#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>

namespace {

void write_file(const char* path, const std::string& content) {
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("unable to open ranking output: " + std::string(path));
    output << content;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 4 && argc != 5) {
        std::cerr << "usage: oasis_re_atlas_rank <retail_rom> <ranking.json> <ranking.txt> [beta_rom]\n";
        return 2;
    }
    try {
        const auto retail = oasis::Rom::load(argv[1]);
        std::optional<oasis::Rom> beta_storage;
        std::optional<std::span<const std::uint8_t>> beta;
        if (argc == 5) {
            beta_storage = oasis::Rom::load(argv[4]);
            beta = beta_storage->bytes();
        }
        const auto atlas = oasis::tools::build_rom_atlas(retail.bytes(), beta);
        const auto ranking = oasis::tools::rank_atlas_unresolved(atlas);
        write_file(argv[2], oasis::tools::ranking_to_json(ranking));
        write_file(argv[3], oasis::tools::ranking_to_text(ranking));
        std::cout << "ranked " << ranking.atlas_unresolved_reference_count << " unresolved references\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
