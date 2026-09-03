#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "tools/re_slice_decoder.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

std::uint32_t parse_u32(const char* text) {
    std::size_t consumed = 0;
    const auto value = std::stoull(text, &consumed, 0);
    if (text[consumed] != '\0' || value > 0xFFFFFFFFULL) {
        throw std::invalid_argument("numeric argument is out of range");
    }
    return static_cast<std::uint32_t>(value);
}

void write_text(const char* path, const std::string& content) {
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("unable to open report output: " + std::string(path));
    output << content;
    if (!output) throw std::runtime_error("unable to write report output: " + std::string(path));
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 4 || argc > 6) {
        std::cerr << "usage: oasis_re_slice <usa_rom> <report.json> <report.txt> "
                     "[entry=0x60004] [byte_budget=0x1200]\n";
        return 2;
    }
    try {
        const auto rom = oasis::Rom::load(argv[1]);
        const auto identity = oasis::identify_rom(rom.bytes());
        if (identity.status != oasis::RomSupportStatus::Supported) {
            throw std::runtime_error("RE slice report requires the supported USA reference ROM");
        }
        oasis::tools::DecodeOptions options{};
        if (argc >= 5) options.entry = parse_u32(argv[4]);
        if (argc == 6) options.byte_budget = parse_u32(argv[5]);
        const auto slice = oasis::tools::decode_m68k_slice(rom.bytes(), options);
        write_text(argv[2], oasis::tools::slice_to_json(slice));
        write_text(argv[3], oasis::tools::slice_to_text(slice));
        std::cout << "decoded " << slice.instructions.size() << " instructions in "
                  << slice.basic_blocks.size() << " basic blocks\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
