#include "core/rom.hpp"
#include "tools/re_candidate_map.hpp"
#include "tools/re_mass_verify.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {

std::string read_file(const char* path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("unable to open Ghidra map: " + std::string(path));
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void write_file(const char* path, const std::string& value) {
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("unable to open mass report: " + std::string(path));
    output << value;
    if (!output) throw std::runtime_error("unable to write mass report: " + std::string(path));
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 5 && argc != 6) {
        std::cerr << "usage: oasis_re_mass_verify <retail_rom> <ghidra.json> <mass.json> <mass.txt> [beta_rom]\n";
        return 2;
    }
    try {
        const auto started = std::chrono::steady_clock::now();
        const auto retail = oasis::Rom::load(argv[1]);
        const auto ghidra = oasis::tools::parse_ghidra_map(read_file(argv[2]));
        std::optional<oasis::Rom> beta_storage;
        std::optional<std::span<const std::uint8_t>> beta;
        if (argc == 6) {
            beta_storage = oasis::Rom::load(argv[5]);
            beta = beta_storage->bytes();
        }
        const auto atlas = oasis::tools::build_rom_atlas(retail.bytes(), beta);
        const auto candidate_map = oasis::tools::build_candidate_map(ghidra, atlas);
        const auto report = oasis::tools::verify_mass_structure(retail.bytes(), candidate_map, atlas);
        write_file(argv[3], oasis::tools::mass_verify_to_json(report));
        write_file(argv[4], oasis::tools::mass_verify_to_text(report));
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started).count();
        std::cout << "verified " << report.total_candidates << " entries in " << elapsed << " ms\n";
        for (const auto& fix : report.top_systemic_fixes)
            std::cout << "fix " << fix.blocker_class << " affected=" << fix.affected_count << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
