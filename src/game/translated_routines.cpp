#include "game/translated_routines.hpp"

namespace oasis::game {

void tilecopy_to_ram(genesis::MemoryBus& bus,
                     std::span<const std::uint8_t> source,
                     std::uint32_t destination) {
    bus.write_ram(destination, source);
}

void tilecopy_to_vram(genesis::MemoryBus& bus,
                      std::span<const std::uint8_t> source,
                      std::uint32_t destination) {
    bus.vdp().write_vram(destination, source);
}

} // namespace oasis::game
