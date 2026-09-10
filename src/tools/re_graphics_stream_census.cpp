#include "core/rom.hpp"
#include "game/graphics_decompress.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <stdexcept>
#include <vector>

namespace {

std::size_t number(const char* text) {
    std::size_t parsed = 0;
    const auto value = std::stoull(text, &parsed, 0);
    if (text[parsed] != '\0') throw std::runtime_error("invalid numeric argument");
    return static_cast<std::size_t>(value);
}

void scan(const std::filesystem::path& rom_path, std::size_t start,
          std::size_t end, const std::filesystem::path& output_path) {
    const auto rom = oasis::Rom::load(rom_path);
    const auto bytes = std::span<const std::uint8_t>(rom.bytes());
    if (start > end || end > bytes.size() || (start & 1U) != 0U) {
        throw std::runtime_error("invalid scan range");
    }
    std::vector<std::uint8_t> decompressed(4U * 1024U * 1024U);
    std::ofstream output(output_path);
    if (!output) throw std::runtime_error("unable to create census output");
    output << "{\n  \"schema\": \"oasis.m12.graphics-stream-census.v1\",\n"
           << "  \"start\": " << start << ",\n  \"end\": " << end
           << ",\n  \"records\": [\n";
    bool first = true;
    std::size_t accepted = 0;
    for (std::size_t candidate = start; candidate + 4U <= end; candidate += 2U) {
        try {
            const auto result = oasis::game::decompress_graphics(
                bytes.subspan(candidate, end - candidate), decompressed);
            const auto stream_end = candidate + result.source_consumed;
            if (stream_end > end || result.source_consumed < 4U ||
                result.output_size == 0U) {
                continue;
            }
            if (!first) output << ",\n";
            first = false;
            output << "    {\"start\": " << candidate << ", \"end\": "
                   << stream_end << ", \"compressed_bytes\": "
                   << result.source_consumed << ", \"decompressed_bytes\": "
                   << result.output_size << "}";
            ++accepted;
        } catch (const std::exception&) {
        }
    }
    output << "\n  ],\n  \"accepted\": " << accepted << "\n}\n";
    std::cout << "accepted=" << accepted << "\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "usage: oasis_graphics_stream_census <rom> <start> <end> <output>\n";
        return 2;
    }
    try {
        scan(argv[1], number(argv[2]), number(argv[3]), argv[4]);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
