#include "core/rom.hpp"
#include "game/graphics_decompress.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Candidate {
    std::size_t start{};
    std::string consumer;
};

std::size_t number(const std::string& text) {
    std::size_t parsed = 0;
    const auto value = std::stoull(text, &parsed, 0);
    if (parsed != text.size()) throw std::runtime_error("invalid candidate number");
    return static_cast<std::size_t>(value);
}

std::vector<Candidate> read_candidates(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("unable to open candidate list");
    std::vector<Candidate> result;
    std::string start;
    std::string consumer;
    while (input >> start >> consumer) {
        result.push_back({number(start), consumer});
    }
    if (result.empty()) throw std::runtime_error("candidate list is empty");
    return result;
}

void scan(const std::filesystem::path& rom_path,
          const std::filesystem::path& candidate_path,
          const std::filesystem::path& output_path) {
    const auto rom = oasis::Rom::load(rom_path);
    const auto bytes = std::span<const std::uint8_t>(rom.bytes());
    const auto candidates = read_candidates(candidate_path);
    std::vector<std::uint8_t> output(4U * 1024U * 1024U);
    std::ofstream report(output_path);
    if (!report) throw std::runtime_error("unable to create scan report");
    report << "{\n  \"schema\": \"oasis.m12.pointer-resource-boundary.v1\",\n"
           << "  \"records\": [\n";
    bool first = true;
    std::size_t accepted = 0;
    for (const auto& candidate : candidates) {
        if (candidate.start >= bytes.size()) continue;
        try {
            const auto decoded = oasis::game::decompress_graphics(
                bytes.subspan(candidate.start), output);
            const auto end = candidate.start + decoded.source_consumed;
            if (!first) report << ",\n";
            first = false;
            report << "    {\"start\": " << candidate.start
                   << ", \"end\": " << end
                   << ", \"compressed_bytes\": " << decoded.source_consumed
                   << ", \"decompressed_bytes\": " << decoded.output_size
                   << ", \"consumer\": \"" << candidate.consumer << "\"}";
            ++accepted;
        } catch (const std::exception&) {
            // A candidate that is not a complete graphics stream remains unowned.
        }
    }
    report << "\n  ],\n  \"accepted\": " << accepted << "\n}\n";
    std::cout << "accepted=" << accepted << "\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: oasis_m12_pointer_resource_scan <rom> <candidates> <output>\n";
        return 2;
    }
    try {
        scan(argv[1], argv[2], argv[3]);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
