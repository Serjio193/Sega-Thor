#include "tools/hybrid/recomp_generator.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void put(std::vector<std::uint8_t>& rom, std::uint32_t address,
         std::initializer_list<std::uint8_t> bytes) {
    std::copy(bytes.begin(), bytes.end(), rom.begin() + address);
}

} // namespace

int main() {
    std::vector<std::uint8_t> rom(0x200, 0);
    put(rom, 0x20, {0x48, 0xE7, 0x01, 0x10, 0x42, 0x47, 0x1E, 0x1E,
                   0x47, 0xF9, 0x00, 0xFF, 0x13, 0x4C, 0xD6, 0xC7,
                   0x1E, 0x1E, 0x36, 0xDE});
    put(rom, 0x80, {0x4D, 0xF9, 0x00, 0xFF, 0x06, 0x28});
    put(rom, 0xA0, {0xD4, 0x81});
    put(rom, 0xC0, {0xF4, 0x00});

    const auto block = oasis::hybrid::generate_block(rom, 0x20, 0x34);
    assert(block.instructions.size() == 7U);
    assert(block.instructions.front().exact->operation == "movem");
    assert(block.instructions.back().exact->operation == "move");
    const auto unit = oasis::hybrid::emit_translation_unit({
        block,
        oasis::hybrid::generate_block(rom, 0x80, 0x86),
        oasis::hybrid::generate_block(rom, 0xA0, 0xA2)});
    assert(unit.find("guest 0x000020 opcode 0x48E7") != std::string::npos);
    assert(unit.find("movem_l_predecrement(api, 0x0880U)") != std::string::npos);
    assert(unit.find("lea_absolute_long(api, 0xFF0628U, 6U)") != std::string::npos);
    assert(unit.find("add_l_data_to_data(api, 1U, 2U)") != std::string::npos);

    bool rejected = false;
    try { (void)oasis::hybrid::generate_block(rom, 0xC0, 0xC2); }
    catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
    return 0;
}
