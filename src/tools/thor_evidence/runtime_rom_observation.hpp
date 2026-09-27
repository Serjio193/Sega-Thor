#pragma once

#include "runtime_rom_properties.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <vector>

namespace oasis::thor::runtime_map {

using PhysicalOffsetResolver = std::function<std::optional<std::uint64_t>(std::uint32_t)>;

class RomOriginMemory {
public:
    explicit RomOriginMemory(std::size_t size);
    [[nodiscard]] std::size_t size() const noexcept { return origins_.size(); }
    [[nodiscard]] std::optional<std::uint32_t> origin(std::size_t address) const;
    void copy_rom_to_ram(std::size_t destination, std::uint64_t rom_offset,
                         std::span<const std::uint8_t> copied_bytes,
                         std::span<const std::uint8_t> rom);
    void copy_ram_to_ram(std::size_t destination, std::size_t source, std::size_t length);
    void invalidate(std::size_t destination, std::size_t length);
private:
    std::vector<std::uint32_t> origins_;
};

// Call only at a completed-instruction hook. Decoder support and byte-by-byte
// ROM mapping are required; otherwise no property is added.
[[nodiscard]] std::optional<std::uint8_t> mark_m68k_executed_encoding(
    PropertyMap& map, std::span<const std::uint8_t> rom, std::uint32_t cpu_pc,
    std::uint16_t executed_opcode, bool instruction_completed,
    const PhysicalOffsetResolver& resolve_rom_offset);

// Call only for a completed CPU data read. The mapper must resolve every byte.
[[nodiscard]] bool mark_resolved_data_read(
    PropertyMap& map, std::span<const std::uint8_t> rom, std::uint32_t cpu_address,
    std::span<const std::uint8_t> observed_bytes, bool completed_data_read,
    std::uint16_t property,
    const PhysicalOffsetResolver& resolve_rom_offset);

struct Z80FetchedByte {
    std::uint8_t value{};
    std::optional<std::uint64_t> rom_origin;
};

// The fetch collector supplies opcode, prefixes and operands in fetch order.
// A single missing/mismatched byte rejects the whole instruction.
[[nodiscard]] bool mark_z80_executed_encoding(
    PropertyMap& map, std::span<const std::uint8_t> rom,
    std::span<const Z80FetchedByte> fetched, bool instruction_completed);

// Call once for each consumed ROM source byte/word after its VDP destination
// write is accepted. Fill and VRAM-copy paths have no ROM source and must not call.
[[nodiscard]] bool mark_vdp_consumed_source(
    PropertyMap& map, std::uint64_t rom_offset, std::uint8_t source_width,
    bool accepted_destination_write, std::uint16_t destination_property);

[[nodiscard]] std::size_t mark_vdp_ram_sources(
    PropertyMap& map, const RomOriginMemory& origins, std::size_t ram_source,
    std::uint8_t source_width, bool accepted_destination_write,
    std::uint16_t destination_property);

} // namespace oasis::thor::runtime_map
