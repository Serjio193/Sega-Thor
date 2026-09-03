#pragma once

#include "genesis/memory_bus.hpp"

#include <cstdint>
#include <span>

namespace oasis::game {

inline constexpr std::uint32_t kDecompressGraphicsAddress = 0x00003820;

void tilecopy_to_ram(genesis::MemoryBus& bus,
                     std::span<const std::uint8_t> source,
                     std::uint32_t destination);

void tilecopy_to_vram(genesis::MemoryBus& bus,
                      std::span<const std::uint8_t> source,
                      std::uint32_t destination);

} // namespace oasis::game
