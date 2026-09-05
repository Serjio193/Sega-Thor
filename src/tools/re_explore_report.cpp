#include "core/rom.hpp"
#include "tools/re_candidate_map.hpp"
#include "tools/re_explore.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>

namespace {
std::string read_file(const char* path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("unable to open candidate map: " + std::string(path));
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void write_file(const char* path, const std::string& value) {
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("unable to open explorer output: " + std::string(path));
    output << value;
    if (!output) throw std::runtime_error("unable to write explorer output: " + std::string(path));
}
} // namespace

int main(int argc, char** argv) {
    if (argc < 5 || argc > 7) {
        std::cerr << "usage: oasis_re_explore <retail_rom> <ghidra.json> <explore.json> <summary.txt> [beta_rom] [--rom-wide]\n";
        return 2;
    }
    try {
        const auto started = std::chrono::steady_clock::now();
        const auto rom = oasis::Rom::load(argv[1]);
        const auto candidates = oasis::tools::parse_ghidra_map(read_file(argv[2]));
        std::optional<oasis::Rom> beta_storage;
        std::optional<std::span<const std::uint8_t>> beta;
        bool wide = false;
        for (int index = 5; index < argc; ++index) {
            if (std::string(argv[index]) == "--rom-wide") wide = true;
            else if (!beta_storage) beta_storage = oasis::Rom::load(argv[index]);
            else throw std::invalid_argument("unknown explorer option");
        }
        if (beta_storage) beta = beta_storage->bytes();
        const auto atlas = oasis::tools::build_rom_atlas(rom.bytes(), beta);
        const auto candidate_map = oasis::tools::build_candidate_map(candidates, atlas);
        oasis::tools::ExploreOptions options;
        options.rom_wide = wide;
        const auto report = oasis::tools::explore_m68k(rom.bytes(), candidate_map, atlas, options);
        write_file(argv[3], oasis::tools::explore_to_json(report));
        write_file(argv[4], oasis::tools::explore_to_text(report));
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started).count();
        std::cout << "explored " << report.metrics.entries_processed << " entries in " << elapsed << " ms\n"
                  << "bounded_control_pass=" << (report.bounded_control_pass ? "yes" : "no")
                  << " rom_wide=" << (report.rom_wide_performed ? "performed" : "not_performed") << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
