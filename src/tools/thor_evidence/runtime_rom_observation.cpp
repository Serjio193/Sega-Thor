#include "runtime_rom_observation.hpp"

#include "tools/re_slice_decoder.hpp"

#include <limits>
#include <stdexcept>
#include <algorithm>
#include <cstring>

namespace oasis::thor::runtime_map {

namespace { constexpr std::uint32_t UNKNOWN_ORIGIN = UINT32_MAX; }

RomOriginMemory::RomOriginMemory(std::size_t size) : origins_(size, UNKNOWN_ORIGIN) {}
std::optional<std::uint32_t> RomOriginMemory::origin(std::size_t address) const {
    const auto value = origins_.at(address);
    return value == UNKNOWN_ORIGIN ? std::nullopt : std::optional<std::uint32_t>(value);
}
void RomOriginMemory::copy_rom_to_ram(std::size_t destination, std::uint64_t rom_offset,
                                      std::span<const std::uint8_t> copied_bytes,
                                      std::span<const std::uint8_t> rom) {
    if (destination > size() || copied_bytes.size() > size() - destination ||
        rom_offset > rom.size() || copied_bytes.size() > rom.size() - rom_offset ||
        rom_offset + copied_bytes.size() > UNKNOWN_ORIGIN)
        throw std::out_of_range("ROM-to-RAM provenance span is out of bounds");
    for (std::size_t i = 0; i < copied_bytes.size(); ++i) {
        origins_[destination + i] = copied_bytes[i] == rom[rom_offset + i]
            ? static_cast<std::uint32_t>(rom_offset + i) : UNKNOWN_ORIGIN;
    }
}
void RomOriginMemory::copy_ram_to_ram(std::size_t destination, std::size_t source,
                                      std::size_t length) {
    if (destination > size() || length > size() - destination || source > size() || length > size() - source)
        throw std::out_of_range("RAM copy provenance span is out of bounds");
    std::memmove(origins_.data() + destination, origins_.data() + source,
                 length * sizeof(origins_[0]));
}
void RomOriginMemory::invalidate(std::size_t destination, std::size_t length) {
    if (destination > size() || length > size() - destination)
        throw std::out_of_range("RAM provenance invalidation span is out of bounds");
    std::fill(origins_.begin() + static_cast<std::ptrdiff_t>(destination),
              origins_.begin() + static_cast<std::ptrdiff_t>(destination + length), UNKNOWN_ORIGIN);
}

std::optional<std::uint8_t> mark_m68k_executed_encoding(
    PropertyMap& map, std::span<const std::uint8_t> rom, std::uint32_t cpu_pc,
    std::uint16_t executed_opcode, bool instruction_completed,
    const PhysicalOffsetResolver& resolve_rom_offset) {
    if (!instruction_completed || !resolve_rom_offset || map.size() != rom.size()) return std::nullopt;
    const auto start = resolve_rom_offset(cpu_pc);
    if (!start || *start > rom.size() || *start > UINT32_MAX) return std::nullopt;
    oasis::tools::DecodedInstruction instruction;
    try {
        instruction = oasis::tools::decode_m68k_instruction(rom, static_cast<std::uint32_t>(*start));
    } catch (const std::invalid_argument&) {
        return std::nullopt;
    }
    const auto length = instruction.bytes.size();
    if (!instruction.supported || instruction.opcode != executed_opcode || length < 2 ||
        length > std::numeric_limits<std::uint8_t>::max() ||
        length > std::numeric_limits<std::uint32_t>::max() - cpu_pc) return std::nullopt;
    for (std::size_t i = 0; i < length; ++i) {
        const auto mapped = resolve_rom_offset(cpu_pc + static_cast<std::uint32_t>(i));
        if (!mapped || *mapped != *start + i) return std::nullopt;
    }
    map.apply_validated(*start, length, M68K_EXECUTED_ENCODING);
    return static_cast<std::uint8_t>(length);
}

bool mark_resolved_data_read(PropertyMap& map, std::span<const std::uint8_t> rom,
                             std::uint32_t cpu_address,
                             std::span<const std::uint8_t> observed_bytes,
                             bool completed_data_read,
                             std::uint16_t property,
                             const PhysicalOffsetResolver& resolve_rom_offset) {
    if (!completed_data_read || !resolve_rom_offset || observed_bytes.empty() ||
        observed_bytes.size() > 4 || map.size() != rom.size() ||
        (property != M68K_DATA_READ && property != Z80_DATA_READ) ||
        observed_bytes.size() > UINT32_MAX - cpu_address) return false;
    const auto start = resolve_rom_offset(cpu_address);
    if (!start || *start > map.size() || observed_bytes.size() > map.size() - *start) return false;
    for (std::size_t i = 0; i < observed_bytes.size(); ++i) {
        const auto mapped = resolve_rom_offset(cpu_address + static_cast<std::uint32_t>(i));
        if (!mapped || *mapped != *start + i || observed_bytes[i] != rom[*mapped]) return false;
    }
    map.apply_validated(*start, observed_bytes.size(), property);
    return true;
}

bool mark_z80_executed_encoding(PropertyMap& map, std::span<const std::uint8_t> rom,
                                std::span<const Z80FetchedByte> fetched,
                                bool instruction_completed) {
    if (!instruction_completed || fetched.empty() || fetched.size() > 256 || map.size() != rom.size())
        return false;
    for (const auto& byte : fetched) {
        if (!byte.rom_origin || *byte.rom_origin >= rom.size() || rom[*byte.rom_origin] != byte.value)
            return false;
    }
    for (const auto& byte : fetched)
        map.apply_validated(*byte.rom_origin, 1, Z80_EXECUTED_ENCODING);
    return true;
}

bool mark_vdp_consumed_source(PropertyMap& map, std::uint64_t rom_offset,
                              std::uint8_t source_width, bool accepted_destination_write,
                              std::uint16_t destination_property) {
    if (!accepted_destination_write || (source_width != 1 && source_width != 2) ||
        (destination_property != VDP_VRAM_SOURCE && destination_property != VDP_CRAM_SOURCE &&
         destination_property != VDP_VSRAM_SOURCE) || rom_offset > map.size() ||
        source_width > map.size() - rom_offset) return false;
    map.apply_validated(rom_offset, source_width, destination_property);
    return true;
}

std::size_t mark_vdp_ram_sources(PropertyMap& map, const RomOriginMemory& origins,
                                std::size_t ram_source, std::uint8_t source_width,
                                bool accepted_destination_write,
                                std::uint16_t destination_property) {
    if (!accepted_destination_write || (source_width != 1 && source_width != 2) ||
        ram_source > origins.size() || source_width > origins.size() - ram_source ||
        (destination_property != VDP_VRAM_SOURCE && destination_property != VDP_CRAM_SOURCE &&
         destination_property != VDP_VSRAM_SOURCE)) return 0;
    std::size_t proven = 0;
    for (std::size_t i = 0; i < source_width; ++i) {
        const auto rom_offset = origins.origin(ram_source + i);
        if (!rom_offset || *rom_offset >= map.size()) continue;
        map.apply_validated(*rom_offset, 1, destination_property);
        ++proven;
    }
    return proven;
}

} // namespace oasis::thor::runtime_map
