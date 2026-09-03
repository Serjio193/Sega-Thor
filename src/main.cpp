#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "genesis/memory_bus.hpp"

#include <exception>
#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: oasis <rom.bin>\n";
        return 1;
    }

    try {
        const auto rom = oasis::Rom::load(argv[1]);
        const auto identity = oasis::identify_rom(rom.bytes());
        oasis::genesis::MemoryBus bus(rom);

        std::cout << "Identity: " << identity.display_name << "\n";
        std::cout << "Status: " << oasis::to_string(identity.status) << "\n";
        std::cout << "Size: " << identity.fingerprint.size << " bytes\n";
        std::cout << "Console: " << identity.header.console_name << "\n";
        std::cout << "Domestic title: " << identity.header.domestic_title << "\n";
        std::cout << "International title: " << identity.header.international_title << "\n";
        std::cout << "Product code: " << identity.header.product_code << "\n";
        std::cout << "Region: " << identity.header.region << "\n";
        std::cout << "Header signature: " << (identity.header.header_signature_valid ? "valid" : "invalid") << "\n";
        std::cout << "Sega checksum: " << (identity.fingerprint.sega_checksum_valid ? "valid" : "invalid") << "\n";
        std::cout << "CRC32: " << std::hex << std::uppercase << std::setw(8) << std::setfill('0')
                  << identity.fingerprint.crc32 << "\n";
        std::cout << "SHA-256: " << identity.fingerprint.sha256 << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return 2;
    }
}
