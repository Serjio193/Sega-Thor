#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "tools/re_assemble.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {

std::uint32_t number(const char* text) {
    std::size_t used = 0;
    const auto value = std::stoull(text, &used, 0);
    if (text[used] || value > 0xFFFFFFFFULL) throw std::invalid_argument("invalid offset");
    return static_cast<std::uint32_t>(value);
}

void write_file(const char* path, const std::string& text) {
    std::ofstream output(path, std::ios::binary);
    if (!output || !(output << text)) throw std::runtime_error("cannot write output");
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 6) {
        std::cerr << "usage: oasis_re_assemble_range <rom> <start> <end> <asm> <json>\n";
        return 2;
    }
    try {
        const auto rom = oasis::Rom::load(argv[1]);
        if (oasis::identify_rom(rom.bytes()).status != oasis::RomSupportStatus::Supported)
            throw std::invalid_argument("canonical supported USA ROM required");
        const auto start = number(argv[2]);
        const auto end = number(argv[3]);
        if ((start & 1U) || start >= end || end > rom.bytes().size())
            throw std::invalid_argument("invalid even bounded range");
        oasis::tools::DecodeOptions options{};
        options.entry = start;
        options.byte_budget = end - start;
        const auto slice = oasis::tools::decode_m68k_slice(rom.bytes(), options);
        if (slice.range_end != end || slice.instructions.empty())
            throw std::invalid_argument("BOUNDARY_UNCERTAIN");
        if (!slice.unsupported_instruction_addresses.empty() ||
            !slice.unresolved_control_flow.empty())
            throw std::invalid_argument("UNSUPPORTED_FORM");
        write_file(argv[4], oasis::tools::slice_asm(slice));
        write_file(argv[5], oasis::tools::exact_slice_json(slice));
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
