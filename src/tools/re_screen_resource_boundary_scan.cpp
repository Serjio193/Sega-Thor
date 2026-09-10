#include "core/rom.hpp"
#include "game/graphics_decompress.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <span>
#include <stdexcept>
#include <vector>

namespace {

constexpr std::size_t kGroupTable = 0xC92C;
constexpr std::size_t kGroupCount = 21;
constexpr std::size_t kResourceCount = 108;

std::uint32_t read_long(std::span<const std::uint8_t> bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 4) {
        throw std::runtime_error("screen group table exceeds ROM");
    }
    return (static_cast<std::uint32_t>(bytes[offset]) << 24U) |
           (static_cast<std::uint32_t>(bytes[offset + 1]) << 16U) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 8U) |
           bytes[offset + 3];
}

std::int16_t read_relative(std::span<const std::uint8_t> bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 2) {
        throw std::runtime_error("screen group entry exceeds ROM");
    }
    const auto word = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(bytes[offset]) << 8U) | bytes[offset + 1]);
    return static_cast<std::int16_t>(word);
}

struct Descriptor {
    std::size_t group{};
    std::size_t index{};
    std::size_t entry{};
    std::size_t address{};
    std::size_t stream{};
};

bool descriptor_candidate(std::span<const std::uint8_t> bytes, std::size_t entry,
                          Descriptor& result, std::size_t group, std::size_t index) {
    if (entry > bytes.size() || bytes.size() - entry < 2) return false;
    const auto target_signed = static_cast<std::int64_t>(entry) + read_relative(bytes, entry);
    if (target_signed < 0) return false;
    const auto target = static_cast<std::size_t>(target_signed);
    if (target > bytes.size() || bytes.size() - target < 26) return false;
    const auto stream = read_long(bytes, target + 4);
    if (stream >= bytes.size()) return false;
    for (std::size_t offset = 8; offset < 12; ++offset) {
        if (bytes[target + offset] >= kResourceCount) return false;
    }
    result = {group, index, entry, target, stream};
    return true;
}

std::vector<Descriptor> descriptors(std::span<const std::uint8_t> bytes) {
    std::vector<Descriptor> result;
    for (std::size_t group = 0; group < kGroupCount; ++group) {
        const auto base = static_cast<std::size_t>(read_long(bytes, kGroupTable + group * 4));
        const auto first = group == 0 ? 1U : 0U; // group zero has a null entry at index zero
        for (std::size_t index = first; index < 0x100; ++index) {
            Descriptor item{};
            if (!descriptor_candidate(bytes, base + index * 2, item, group, index)) break;
            result.push_back(item);
        }
    }
    return result;
}

void write_scan(const std::filesystem::path& rom_path,
                const std::filesystem::path& output_path) {
    const auto rom = oasis::Rom::load(rom_path);
    const auto bytes = std::span<const std::uint8_t>(rom.bytes());
    const auto all_descriptors = descriptors(bytes);
    std::map<std::size_t, std::vector<Descriptor>> uses;
    for (const auto& descriptor : all_descriptors) uses[descriptor.stream].push_back(descriptor);

    std::vector<std::uint8_t> decompressed(4U * 1024U * 1024U);
    std::ofstream output(output_path);
    if (!output) throw std::runtime_error("unable to create scan output");
    output << "{\n  \"schema\": \"oasis.m68k.screen-resource-boundary.v1\",\n"
           << "  \"group_table\": 51628,\n  \"descriptors\": "
           << all_descriptors.size() << ",\n  \"records\": [\n";
    bool first = true;
    std::size_t accepted = 0;
    for (const auto& [stream, stream_uses] : uses) {
        std::size_t consumed = 0;
        std::size_t produced = 0;
        std::string status = "ACCEPTED";
        std::string error;
        try {
            const auto result = oasis::game::decompress_graphics(bytes.subspan(stream), decompressed);
            consumed = result.source_consumed;
            produced = result.output_size;
            if (stream + consumed > bytes.size()) status = "REJECTED_BOUNDARY";
        } catch (const std::exception& exception) {
            status = "REJECTED_DECODE";
            error = exception.what();
        }
        if (!first) output << ",\n";
        first = false;
        output << "    {\"start\": " << stream << ", \"end\": " << stream + consumed
               << ", \"compressed_bytes\": " << consumed
               << ", \"decompressed_bytes\": " << produced
               << ", \"status\": \"" << status << "\", \"uses\": [";
        for (std::size_t i = 0; i < stream_uses.size(); ++i) {
            if (i != 0) output << ", ";
            output << "{\"group\": " << stream_uses[i].group
                   << ", \"index\": " << stream_uses[i].index
                   << ", \"descriptor\": " << stream_uses[i].address << "}";
        }
        output << "]";
        if (!error.empty()) output << ", \"error\": \"" << error << "\"";
        output << "}";
        if (status == "ACCEPTED") ++accepted;
    }
    output << "\n  ],\n  \"unique_streams\": " << uses.size()
           << ",\n  \"accepted\": " << accepted << "\n}\n";
    std::cout << "descriptors=" << all_descriptors.size() << " streams=" << uses.size()
              << " accepted=" << accepted << "\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: oasis_screen_resource_boundary_scan <rom> <output>\n";
        return 2;
    }
    try {
        write_scan(argv[1], argv[2]);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
