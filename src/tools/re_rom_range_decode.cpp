#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "tools/hybrid/address_provenance.hpp"
#include "tools/re_slice_decoder.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::uint32_t parse_u32(const std::string& text) {
    std::size_t consumed = 0;
    const auto value = std::stoull(text, &consumed, 0);
    if (consumed != text.size() || value > 0xFFFFFFFFULL)
        throw std::invalid_argument("invalid 32-bit CPU PC");
    return static_cast<std::uint32_t>(value);
}

struct Resolution {
    std::uint32_t cpu_address{};
    oasis::hybrid::MemoryClass memory_region{oasis::hybrid::MemoryClass::UNKNOWN};
    std::optional<std::uint32_t> rom_offset;
    std::string rom_sha256;
    std::string decode_status{"NOT_ROM_BACKED"};
    std::optional<std::uint16_t> opcode;
    std::vector<std::uint8_t> bytes;
};

Resolution resolve_and_decode(std::span<const std::uint8_t> rom,
                              std::uint32_t cpu_pc,
                              const std::string& rom_sha256) {
    Resolution result{};
    result.cpu_address = cpu_pc & 0x00FFFFFFU;
    result.memory_region = oasis::hybrid::classify_memory_address(
        result.cpu_address, static_cast<std::uint32_t>(rom.size()));
    if (result.memory_region != oasis::hybrid::MemoryClass::ROM) return result;

    result.rom_offset = result.cpu_address;
    result.rom_sha256 = rom_sha256;
    if ((result.cpu_address & 1U) != 0U ||
        rom.size() - result.cpu_address < 2U) {
        result.decode_status = "DECODE_UNSUPPORTED";
        return result;
    }

    oasis::tools::DecodeOptions options{};
    options.entry = result.cpu_address;
    options.byte_budget = std::min<std::size_t>(16U, rom.size() - result.cpu_address);
    options.instruction_budget = 1U;
    const auto decoded = oasis::tools::decode_m68k_slice(rom, options);
    if (decoded.instructions.size() != 1U) {
        result.decode_status = "DECODE_UNSUPPORTED";
        return result;
    }
    const auto& instruction = decoded.instructions.front();
    result.opcode = instruction.opcode;
    result.bytes = instruction.bytes;
    result.decode_status = instruction.supported ? "DECODED" : "DECODE_UNSUPPORTED";
    return result;
}

std::string hex_bytes(const std::vector<std::uint8_t>& bytes) {
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (const auto byte : bytes) output << std::setw(2) << static_cast<unsigned>(byte);
    return output.str();
}

void emit(std::ostream& output, std::uint32_t cpu_pc, const Resolution& value) {
    output << "0x" << std::hex << std::setw(8) << std::setfill('0') << cpu_pc
           << '\t' << "0x" << std::setw(8) << value.cpu_address << '\t'
           << oasis::hybrid::memory_class_name(value.memory_region) << '\t';
    if (value.rom_offset) output << "0x" << std::setw(8) << *value.rom_offset;
    output << '\t' << value.rom_sha256 << '\t' << value.decode_status << '\t';
    if (value.opcode) output << "0x" << std::setw(4) << *value.opcode;
    output << '\t' << std::dec << value.bytes.size() << '\t'
           << hex_bytes(value.bytes) << '\n';
}

void self_test() {
    std::vector<std::uint8_t> rom(64U, 0x4EU);
    for (std::size_t offset = 0; offset + 1U < rom.size(); offset += 2U)
        rom[offset + 1U] = 0x71U;
    rom[0] = 0x48U; rom[1] = 0xE7U; rom[2] = 0x30U; rom[3] = 0x30U;
    rom[4] = 0x70U; rom[5] = 0x01U;
    rom[6] = 0xF0U; rom[7] = 0x00U;
    const auto rom_sha256 = oasis::calculate_sha256(rom);
    const auto extended = resolve_and_decode(rom, 0x01000000U, rom_sha256);
    if (extended.memory_region != oasis::hybrid::MemoryClass::ROM ||
        extended.rom_offset != 0U || extended.decode_status != "DECODED" ||
        extended.rom_sha256 != rom_sha256 ||
        extended.opcode != 0x48E7U || extended.bytes !=
            std::vector<std::uint8_t>{0x48U, 0xE7U, 0x30U, 0x30U})
        throw std::runtime_error("extended instruction decode or 24-bit ROM resolution failed");
    const auto short_instruction = resolve_and_decode(rom, 4U, rom_sha256);
    if (short_instruction.decode_status != "DECODED" ||
        short_instruction.bytes != std::vector<std::uint8_t>{0x70U, 0x01U})
        throw std::runtime_error("two-byte instruction extent failed");
    const auto unsupported = resolve_and_decode(rom, 6U, rom_sha256);
    if (unsupported.decode_status != "DECODE_UNSUPPORTED")
        throw std::runtime_error("unsupported instruction was not fail-closed");
    const auto ram = resolve_and_decode(rom, 0x00FF0000U, rom_sha256);
    if (ram.memory_region != oasis::hybrid::MemoryClass::MAIN_RAM || ram.rom_offset ||
        !ram.rom_sha256.empty())
        throw std::runtime_error("RAM PC was falsely mapped to ROM");
    const auto outside = resolve_and_decode(rom, 0x00000040U, rom_sha256);
    if (outside.memory_region != oasis::hybrid::MemoryClass::UNKNOWN || outside.rom_offset ||
        !outside.rom_sha256.empty())
        throw std::runtime_error("unmapped PC was falsely mapped to ROM");
}

int run(const char* rom_path, const char* input_path, const char* output_path) {
    const auto rom = oasis::Rom::load(rom_path);
    const auto identity = oasis::identify_rom(rom.bytes());
    if (identity.status != oasis::RomSupportStatus::Supported ||
        identity.fingerprint.sha256 != "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263")
        throw std::invalid_argument("canonical supported USA ROM required");
    std::ifstream input(input_path);
    std::ofstream output(output_path, std::ios::binary);
    if (!input || !output) throw std::runtime_error("unable to open decoder input/output");
    std::string line;
    std::uint64_t count = 0;
    while (std::getline(input, line)) {
        if (line.empty()) continue;
        const auto raw_pc = parse_u32(line);
        emit(output, raw_pc, resolve_and_decode(rom.bytes(), raw_pc,
             identity.fingerprint.sha256));
        ++count;
    }
    if (!input.eof() || !output) throw std::runtime_error("decoder batch I/O failed");
    std::cout << "decoded/resolved " << count << " CPU addresses\n";
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--self-test") {
            self_test();
            return 0;
        }
        if (argc != 4) {
            std::cerr << "usage: oasis_re_rom_range_decode <usa_rom> <pcs.txt> <results.tsv>\n";
            return 2;
        }
        return run(argv[1], argv[2], argv[3]);
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
