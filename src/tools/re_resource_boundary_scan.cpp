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

std::uint32_t read_long(std::span<const std::uint8_t> bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 4) {
        throw std::runtime_error("pointer table exceeds ROM");
    }
    return (static_cast<std::uint32_t>(bytes[offset]) << 24U) |
           (static_cast<std::uint32_t>(bytes[offset + 1]) << 16U) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 8U) |
           bytes[offset + 3];
}

void scan(const std::filesystem::path& rom_path, std::size_t table_start,
          std::size_t count, const std::filesystem::path& output_path) {
    const auto rom = oasis::Rom::load(rom_path);
    const auto bytes = std::span<const std::uint8_t>(rom.bytes());
    std::vector<std::uint8_t> decompressed(4U * 1024U * 1024U);
    std::ofstream output(output_path);
    if (!output) throw std::runtime_error("unable to create scan output");
    output << "{\n  \"table_start\": " << table_start << ",\n"
           << "  \"count\": " << count << ",\n  \"records\": [\n";
    bool first = true;
    std::size_t accepted = 0;
    for (std::size_t index = 0; index < count; ++index) {
        const auto start = read_long(bytes, table_start + index * 4U);
        const auto next = index + 1U < count
            ? read_long(bytes, table_start + (index + 1U) * 4U) : 0U;
        if (start == 0U) continue;
        if (start >= bytes.size() || (next != 0U && next <= start) ||
            (next != 0U && next > bytes.size())) continue;
        const auto result = oasis::game::decompress_graphics(
            bytes.subspan(start), decompressed);
        const auto end = start + result.source_consumed;
        const bool boundary_agrees = next == 0U || end <= next;
        if (!boundary_agrees) continue;
        if (!first) output << ",\n";
        first = false;
        output << "    {\"index\": " << index << ", \"start\": " << start
               << ", \"end\": " << end << ", \"compressed_bytes\": "
               << result.source_consumed << ", \"decompressed_bytes\": "
               << result.output_size << ", \"boundary\": \""
               << (next == 0U ? "stream_end" : end == next ? "next_pointer" : "padded_before_next_pointer")
               << "\", \"padding_after_stream\": "
               << (next == 0U ? 0U : next - end) << "}";
        ++accepted;
    }
    output << "\n  ],\n  \"accepted\": " << accepted << "\n}\n";
    std::cout << "accepted=" << accepted << "\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "usage: oasis_resource_boundary_scan <rom> <table> <count> <output>\n";
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
