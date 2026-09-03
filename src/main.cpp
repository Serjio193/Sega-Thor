#include "core/rom.hpp"
#include "genesis/memory_bus.hpp"

#include <exception>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: oasis <rom.bin>\n";
        return 1;
    }

    try {
        const auto rom = oasis::Rom::load(argv[1]);
        oasis::genesis::MemoryBus bus(rom);

        std::cout << "Loaded ROM: " << rom.size() << " bytes\n";
        std::cout << "Domestic title: " << rom.domestic_title() << "\n";
        std::cout << "International title: " << rom.international_title() << "\n";
        std::cout << "VDP data port: 0xC00000\n";
        std::cout << "VDP control port: 0xC00004\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return 2;
    }
}
