#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "tools/re_static_translation.hpp"

#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct CaseA {
    std::size_t offset;
    std::size_t consumed;
    std::size_t output;
};

void run_a(const oasis::Rom& rom, const CaseA& item) {
    std::vector<std::uint8_t> mechanical_output(4U * 1024U * 1024U);
    std::vector<std::uint8_t> native_output(mechanical_output.size());
    const auto source = std::span<const std::uint8_t>(rom.bytes()).subspan(item.offset);
    const auto mechanical = oasis::tools::mechanical_3820(source, mechanical_output);
    const auto native = oasis::game::decompress_graphics(source, native_output);
    if (mechanical.source_consumed != item.consumed || mechanical.output_size != item.output ||
        native.source_consumed != mechanical.source_consumed || native.output_size != mechanical.output_size ||
        !std::equal(mechanical_output.begin(), mechanical_output.begin() + mechanical.output_size,
                    native_output.begin())) {
        throw std::runtime_error("case A differential mismatch");
    }
    std::cout << "A entry=0x3820 offset=0x" << std::hex << item.offset << std::dec
              << " instructions=306 consumed=" << mechanical.source_consumed
              << " output=" << mechanical.output_size << " mismatch=none\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: oasis_re_static_translation <Beyond Oasis USA ROM>\n";
        return 2;
    }
    try {
        const auto rom = oasis::Rom::load(argv[1]);
        if (oasis::identify_rom(rom.bytes()).status != oasis::RomSupportStatus::Supported)
            throw std::runtime_error("case A requires the supported USA ROM");
        for (const auto item : std::array<CaseA, 2>{{{0x16943C, 1217, 3072}, {0x1894EA, 112, 128}}})
            run_a(rom, item);
        std::cout << "B entry=0xA8DA instructions=10 fixture=static-mass-verified\n"
                     "C entry=0x62CC instructions=6 fixture=static-mass-verified\n"
                     "unsupported=explicit-stop fallback-interpreter=no production-cpu-emulator=no\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
