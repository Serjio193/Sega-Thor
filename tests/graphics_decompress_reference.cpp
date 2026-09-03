#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "game/graphics_decompress.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

struct ReferenceCase {
    std::size_t source_offset;
    std::size_t source_consumed;
    std::size_t output_size;
    std::string_view output_sha256;
};

void verify_case(const oasis::Rom& rom, const ReferenceCase& expected) {
    if (expected.source_offset >= rom.size()) throw std::runtime_error("reference offset outside ROM");

    std::vector<std::uint8_t> output(4U * 1024U * 1024U);
    const auto source = std::span<const std::uint8_t>(rom.bytes()).subspan(expected.source_offset);
    const auto result = oasis::game::decompress_graphics(source, output);
    const auto hash = oasis::calculate_sha256(
        std::span<const std::uint8_t>(output.data(), result.output_size));

    if (result.source_consumed != expected.source_consumed ||
        result.output_size != expected.output_size ||
        hash != expected.output_sha256) {
        std::cerr << "reference mismatch at ROM offset 0x" << std::hex << expected.source_offset
                  << std::dec << "\n"
                  << "consumed: " << result.source_consumed << " expected " << expected.source_consumed << "\n"
                  << "output:   " << result.output_size << " expected " << expected.output_size << "\n"
                  << "sha256:   " << hash << " expected " << expected.output_sha256 << "\n";
        throw std::runtime_error("native decompressor differs from original 68000 trace");
    }

    std::cout << "verified offset=0x" << std::hex << expected.source_offset << std::dec
              << " consumed=" << result.source_consumed
              << " output=" << result.output_size
              << " sha256=" << hash << "\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: oasis_graphics_reference <Beyond Oasis USA ROM>\n";
        return 2;
    }

    try {
        const auto rom = oasis::Rom::load(argv[1]);
        const auto identity = oasis::identify_rom(rom.bytes());
        if (identity.status != oasis::RomSupportStatus::Supported) {
            throw std::runtime_error("reference test requires the supported USA ROM");
        }

        constexpr std::array cases{
            ReferenceCase{
                0x16943CU,
                1217U,
                3072U,
                "65e99e74020fedbdcb97c8249a5ccfe540aca5bb5d29bfb260352cd6f388c31a"},
            ReferenceCase{
                0x1894EAU,
                112U,
                128U,
                "167d4e5409f6b075b3b6f2bc61dbb747e8d8c857e8699745184ddf48d83bcda9"},
        };

        for (const auto& test : cases) verify_case(rom, test);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
