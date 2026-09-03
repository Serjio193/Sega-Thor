#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "game/genesis_graphics.hpp"
#include "game/graphics_decompress.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::size_t parse_number(const char* text) {
    std::size_t parsed = 0;
    const auto value = std::stoull(text, &parsed, 0);
    if (text[parsed] != '\0') throw std::runtime_error("invalid numeric argument");
    return static_cast<std::size_t>(value);
}

std::vector<std::uint8_t> build_tile_sheet(std::span<const std::uint8_t> tile_bytes,
                                           std::size_t columns,
                                           std::size_t& width,
                                           std::size_t& height) {
    const std::size_t tile_count = tile_bytes.size() / 32U;
    if (tile_count == 0) throw std::runtime_error("decompressed data contains no complete Genesis tiles");
    if (columns == 0) throw std::runtime_error("tile columns must be greater than zero");

    columns = std::min(columns, tile_count);
    const std::size_t rows = (tile_count + columns - 1U) / columns;
    width = columns * 8U;
    height = rows * 8U;
    std::vector<std::uint8_t> image(width * height, 0);

    for (std::size_t tile_index = 0; tile_index < tile_count; ++tile_index) {
        const auto tile = oasis::game::decode_genesis_4bpp_tile(
            tile_bytes.subspan(tile_index * 32U, 32U));
        const std::size_t tile_x = (tile_index % columns) * 8U;
        const std::size_t tile_y = (tile_index / columns) * 8U;
        for (std::size_t y = 0; y < 8; ++y) {
            for (std::size_t x = 0; x < 8; ++x) {
                image[(tile_y + y) * width + tile_x + x] = tile[y * 8U + x];
            }
        }
    }
    return image;
}

void ensure_parent_directory(const std::filesystem::path& output_path) {
    const auto parent = output_path.parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent);
}

void write_pgm(const std::filesystem::path& path,
               std::span<const std::uint8_t> indices,
               std::size_t width,
               std::size_t height) {
    ensure_parent_directory(path);
    std::ofstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("unable to create PGM output");
    file << "P5\n" << width << ' ' << height << "\n255\n";
    for (const auto index : indices) {
        const auto gray = static_cast<std::uint8_t>(index * 17U);
        file.put(static_cast<char>(gray));
    }
}

void write_ppm(const std::filesystem::path& path,
               std::span<const std::uint8_t> indices,
               std::size_t width,
               std::size_t height,
               const oasis::game::Palette16& palette) {
    ensure_parent_directory(path);
    std::ofstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("unable to create PPM output");
    file << "P6\n" << width << ' ' << height << "\n255\n";
    for (const auto index : indices) {
        const auto color = palette[index & 0x0FU];
        file.put(static_cast<char>(color.r));
        file.put(static_cast<char>(color.g));
        file.put(static_cast<char>(color.b));
    }
}

void print_usage() {
    std::cerr << "usage: oasis_inspect <rom> <gfx_offset> <output> [palette_offset] [columns]\n"
              << "  no palette: writes indexed/grayscale PGM\n"
              << "  palette:    reads 16 CRAM colors at palette_offset and writes RGB PPM\n"
              << "  offsets accept decimal or 0x-prefixed hexadecimal\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 4 || argc > 6) {
        print_usage();
        return 2;
    }

    try {
        const auto rom = oasis::Rom::load(argv[1]);
        const auto identity = oasis::identify_rom(rom.bytes());
        if (identity.status != oasis::RomSupportStatus::Supported) {
            throw std::runtime_error("address-based inspection requires the supported USA reference ROM");
        }

        const auto gfx_offset = parse_number(argv[2]);
        if (gfx_offset >= rom.size()) throw std::runtime_error("graphics offset is outside ROM");

        const std::size_t columns = argc >= 6 ? parse_number(argv[5]) : 16U;
        std::vector<std::uint8_t> decompressed(4U * 1024U * 1024U);
        const auto source = std::span<const std::uint8_t>(rom.bytes()).subspan(gfx_offset);
        const auto result = oasis::game::decompress_graphics(source, decompressed);
        const auto complete_tile_bytes = (result.output_size / 32U) * 32U;

        std::size_t width = 0;
        std::size_t height = 0;
        const auto sheet = build_tile_sheet(
            std::span<const std::uint8_t>(decompressed.data(), complete_tile_bytes),
            columns,
            width,
            height);

        const std::filesystem::path output_path = argv[3];
        if (argc >= 5) {
            const auto palette_offset = parse_number(argv[4]);
            if (palette_offset > rom.size() || rom.size() - palette_offset < 32U) {
                throw std::runtime_error("palette offset does not contain 32 ROM bytes");
            }
            const auto palette = oasis::game::decode_genesis_palette16(
                std::span<const std::uint8_t>(rom.bytes()).subspan(palette_offset, 32U));
            write_ppm(output_path, sheet, width, height, palette);
        } else {
            write_pgm(output_path, sheet, width, height);
        }

        std::cout << "source_offset=0x" << std::hex << gfx_offset << std::dec << '\n'
                  << "source_consumed=" << result.source_consumed << '\n'
                  << "decompressed_bytes=" << result.output_size << '\n'
                  << "complete_tiles=" << (complete_tile_bytes / 32U) << '\n'
                  << "sheet=" << width << 'x' << height << '\n'
                  << "output=" << output_path.string() << '\n';
        if (result.output_size != complete_tile_bytes) {
            std::cout << "trailing_non_tile_bytes=" << (result.output_size - complete_tile_bytes) << '\n';
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
