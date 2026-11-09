#include "tools/thor_ir/portable_guest_env.hpp"

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>

#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed at " << __FILE__ << ":" << __LINE__ << ": " #cond << "\n"; \
            std::exit(1); \
        } \
    } while (false)

namespace {

using namespace oasis::recompiler::ir;

void test_full_canonical_rom_mapping() {
    std::ifstream rom_file("local-roms/Beyond Oasis (USA).md", std::ios::binary);
    TEST_ASSERT(rom_file.is_open());
    std::vector<std::uint8_t> rom_bytes((std::istreambuf_iterator<char>(rom_file)),
                                         std::istreambuf_iterator<char>());
    TEST_ASSERT(rom_bytes.size() == 3145728U); // Exactly 3 MiB

    PortableGuestEnvironment env;
    env.set_rom(rom_bytes);

    // 1. Near 0x000000: Initial SSP vector
    const auto val_000000 = env.read_memory(0x000000U, IrOpSize::Word);
    TEST_ASSERT(val_000000 == 0x00FFU);
    const auto val_000004 = env.read_memory(0x000004U, IrOpSize::Long);
    TEST_ASSERT(val_000004 == 0x0000020EU); // Initial PC (0x00020E)

    // 2. Near 0x1FFFFE: End of 2nd MiB (Word read)
    const auto val_1ffffe = env.read_memory(0x1FFFFEU, IrOpSize::Word);
    TEST_ASSERT(val_1ffffe == 0x65E0U);

    // 3. Near 0x200000: Start of 3rd MiB (Word & Byte read)
    const auto val_200000_w = env.read_memory(0x200000U, IrOpSize::Word);
    TEST_ASSERT(val_200000_w == 0xFE63U);
    const auto val_200000_b = env.read_memory(0x200000U, IrOpSize::Byte);
    TEST_ASSERT(val_200000_b == 0xFEU);
    const auto val_200001_b = env.read_memory(0x200001U, IrOpSize::Byte);
    TEST_ASSERT(val_200001_b == 0x63U);

    // 4. Near 0x2FFFFE: Last word of 3rd MiB
    const auto val_2ffffe = env.read_memory(0x2FFFFEU, IrOpSize::Word);
    TEST_ASSERT(val_2ffffe == 0xFFFFU);

    // 5. Outside valid ROM range: 0x300000 (Start of 4th unmapped MiB)
    TEST_ASSERT(!env.bus_error);
    const auto unmapped_val = env.read_memory(0x300000U, IrOpSize::Word);
    TEST_ASSERT(env.bus_error);
    TEST_ASSERT(unmapped_val == 0xFFFFU); // Open bus pull-up

    // 6. Address 0x3FFFFE: End of cart window
    env.bus_error = false;
    const auto unmapped_end = env.read_memory(0x3FFFFEU, IrOpSize::Word);
    TEST_ASSERT(env.bus_error);
    TEST_ASSERT(unmapped_end == 0xFFFFU);

    std::cout << "PASS: test_full_canonical_rom_mapping (0x000000, 0x1FFFFE, 0x200000, 0x2FFFFE, 0x300000)\n";
}

void test_rom_read_only_protection() {
    std::vector<std::uint8_t> rom(kCanonicalRomSize, 0xAAU);
    PortableGuestEnvironment env;
    env.set_rom(rom);

    TEST_ASSERT(!env.rom_write_violation);
    env.write_memory(0x002000U, IrOpSize::Byte, 0x55U);
    TEST_ASSERT(env.rom_write_violation);

    // Assert ROM byte remained unchanged
    const auto read_back = env.read_memory(0x002000U, IrOpSize::Byte);
    TEST_ASSERT(read_back == 0xAAU);

    // Write Word to 3rd MiB ROM
    env.rom_write_violation = false;
    env.write_memory(0x250000U, IrOpSize::Word, 0x1234U);
    TEST_ASSERT(env.rom_write_violation);
    TEST_ASSERT(env.read_memory(0x250000U, IrOpSize::Word) == 0xAAAAU);

    std::cout << "PASS: test_rom_read_only_protection\n";
}

void test_ram_mapping_and_mirroring() {
    PortableGuestEnvironment env;

    // Direct write to Work RAM base
    env.write_memory(0x00FF1234U, IrOpSize::Word, 0xBEEFU);
    TEST_ASSERT(env.read_memory(0x00FF1234U, IrOpSize::Word) == 0xBEEFU);

    // Read via Genesis Work RAM mirror (0x00E01234)
    TEST_ASSERT(env.read_memory(0x00E01234U, IrOpSize::Word) == 0xBEEFU);

    // Write via mirror, read via standard base
    env.write_memory(0x00E55678U, IrOpSize::Long, 0x12345678U);
    TEST_ASSERT(env.read_memory(0x00FF5678U, IrOpSize::Long) == 0x12345678U);

    // 24-bit address masking
    TEST_ASSERT(env.read_memory(0x01FF1234U, IrOpSize::Word) == 0xBEEFU);

    std::cout << "PASS: test_ram_mapping_and_mirroring\n";
}

void test_endianness_and_alignment_exceptions() {
    PortableGuestEnvironment env;

    // Big-endian Long write and byte-by-byte verification
    env.write_memory(0x00FF0000U, IrOpSize::Long, 0x11223344U);
    TEST_ASSERT(env.read_memory(0x00FF0000U, IrOpSize::Byte) == 0x11U);
    TEST_ASSERT(env.read_memory(0x00FF0001U, IrOpSize::Byte) == 0x22U);
    TEST_ASSERT(env.read_memory(0x00FF0002U, IrOpSize::Byte) == 0x33U);
    TEST_ASSERT(env.read_memory(0x00FF0003U, IrOpSize::Byte) == 0x44U);

    // Address Error exception on odd-address Word read
    TEST_ASSERT(!env.address_error);
    const auto odd_word = env.read_memory(0x00FF0001U, IrOpSize::Word);
    TEST_ASSERT(env.address_error);
    TEST_ASSERT(odd_word == 0);

    // Address Error exception on odd-address Long write
    env.address_error = false;
    env.write_memory(0x00FF0003U, IrOpSize::Long, 0x55667788U);
    TEST_ASSERT(env.address_error);

    std::cout << "PASS: test_endianness_and_alignment_exceptions\n";
}

void test_mmio_callbacks() {
    PortableGuestEnvironment env;
    std::uint32_t last_mmio_addr = 0;
    std::uint32_t last_mmio_val = 0;
    bool last_mmio_is_write = false;

    env.mmio_handler = [&](std::uint32_t a, IrOpSize, std::uint32_t v, bool w) {
        last_mmio_addr = a;
        last_mmio_val = v;
        last_mmio_is_write = w;
    };

    // VDP Data port write (0xC00000)
    env.write_memory(0x00C00000U, IrOpSize::Word, 0xABCDU);
    TEST_ASSERT(last_mmio_addr == 0x00C00000U);
    TEST_ASSERT(last_mmio_val == 0xABCDU);
    TEST_ASSERT(last_mmio_is_write == true);

    // Z80 RAM read (0xA00100)
    env.read_memory(0x00A00100U, IrOpSize::Byte);
    TEST_ASSERT(last_mmio_addr == 0x00A00100U);
    TEST_ASSERT(last_mmio_is_write == false);

    std::cout << "PASS: test_mmio_callbacks\n";
}

} // namespace

int main() {
    std::cout << "=== Running M15.241 Portable Memory Map Tests ===\n";
    test_full_canonical_rom_mapping();
    test_rom_read_only_protection();
    test_ram_mapping_and_mirroring();
    test_endianness_and_alignment_exceptions();
    test_mmio_callbacks();
    std::cout << "ALL PORTABLE MEMORY MAP TESTS PASSED!\n";
    return 0;
}
