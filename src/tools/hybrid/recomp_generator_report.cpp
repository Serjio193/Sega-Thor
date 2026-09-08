#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "tools/hybrid/recomp_generator.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

std::uint32_t number(const char* text) {
    std::size_t used = 0;
    const auto value = std::stoull(text, &used, 0);
    if (text[used] || value > 0xFFFFFFFFULL)
        throw std::invalid_argument("invalid block address");
    return static_cast<std::uint32_t>(value);
}

void write_file(const char* path, const std::string& content) {
    std::ofstream output(path, std::ios::binary);
    if (!output || !(output << content))
        throw std::runtime_error("unable to write generated output");
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 5 || ((argc - 3) & 1) != 0) {
        std::cerr << "usage: oasis_hybrid_recomp_generate <usa_rom> <out.cpp> "
                     "<start> <end> [<start> <end> ...]\n";
        return 2;
    }
    try {
        const auto rom = oasis::Rom::load(argv[1]);
        if (oasis::identify_rom(rom.bytes()).status != oasis::RomSupportStatus::Supported)
            throw std::invalid_argument("canonical supported USA ROM required");
        std::vector<oasis::hybrid::GeneratedBlock> blocks;
        for (int i = 3; i < argc; i += 2)
            blocks.push_back(oasis::hybrid::generate_block(rom.bytes(), number(argv[i]),
                                                           number(argv[i + 1])));
        write_file(argv[2], oasis::hybrid::emit_translation_unit(blocks));
        std::cout << "generated " << blocks.size() << " decoder-owned blocks\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
