#include "core/rom_identity.hpp"

#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace {

void write_text(std::vector<std::uint8_t>& rom, std::size_t offset, std::string_view text, std::size_t width) {
    for (std::size_t i = 0; i < width; ++i) {
        rom[offset + i] = i < text.size() ? static_cast<std::uint8_t>(text[i]) : static_cast<std::uint8_t>(' ');
    }
}

void write_be16(std::vector<std::uint8_t>& rom, std::size_t offset, std::uint16_t value) {
    rom[offset] = static_cast<std::uint8_t>(value >> 8U);
    rom[offset + 1] = static_cast<std::uint8_t>(value);
}

void write_be32(std::vector<std::uint8_t>& rom, std::size_t offset, std::uint32_t value) {
    rom[offset] = static_cast<std::uint8_t>(value >> 24U);
    rom[offset + 1] = static_cast<std::uint8_t>(value >> 16U);
    rom[offset + 2] = static_cast<std::uint8_t>(value >> 8U);
    rom[offset + 3] = static_cast<std::uint8_t>(value);
}

std::vector<std::uint8_t> bytes_of(std::string_view text) {
    return std::vector<std::uint8_t>(text.begin(), text.end());
}

} // namespace

int main() {
    const auto crc_bytes = bytes_of("123456789");
    assert(oasis::calculate_crc32(crc_bytes) == 0xCBF43926U);

    const auto sha_bytes = bytes_of("abc");
    assert(oasis::calculate_sha1(sha_bytes) ==
           "a9993e364706816aba3e25717850c26c9cd0d89d");
    assert(oasis::calculate_sha256(sha_bytes) ==
           "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

    std::vector<std::uint8_t> rom(0x400, 0);
    write_text(rom, 0x100, "SEGA GENESIS", 16);
    write_text(rom, 0x120, "SYNTHETIC TEST GAME", 48);
    write_text(rom, 0x150, "SYNTHETIC TEST GAME", 48);
    write_text(rom, 0x180, "GM TEST-0000", 14);
    write_be32(rom, 0x1A0, 0x00000000U);
    write_be32(rom, 0x1A4, 0x000003FFU);
    write_text(rom, 0x1F0, "JUE", 16);

    rom[0x200] = 0x12;
    rom[0x201] = 0x34;
    rom[0x202] = 0xAB;
    rom[0x203] = 0xCD;
    const auto checksum = oasis::calculate_sega_checksum(rom);
    assert(checksum == static_cast<std::uint16_t>(0x1234U + 0xABCDU));
    write_be16(rom, 0x18E, checksum);

    const auto header = oasis::parse_mega_drive_header(rom);
    assert(header.header_signature_valid);
    assert(header.domestic_title == "SYNTHETIC TEST GAME");
    assert(header.product_code == "GM TEST-0000");
    assert(header.region == "JUE");
    assert(header.rom_start == 0x00000000U);
    assert(header.rom_end == 0x000003FFU);

    const auto identity = oasis::identify_rom(rom);
    assert(identity.status == oasis::RomSupportStatus::Unknown);
    assert(identity.fingerprint.sega_checksum_valid);
    assert(identity.fingerprint.size == rom.size());
    return 0;
}
