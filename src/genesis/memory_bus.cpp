#include "genesis/memory_bus.hpp"

#include <algorithm>
#include <stdexcept>

namespace oasis::genesis {

std::uint8_t MemoryBus::read8(std::uint32_t address) const {
    address &= 0x00FFFFFFu;
    if (address < rom_.size()) return rom_.bytes()[address];
    if (address >= kRamBase) return ram_[address - kRamBase];
    throw std::out_of_range("Unmapped Mega Drive read");
}

void MemoryBus::write8(std::uint32_t address, std::uint8_t value) {
    address &= 0x00FFFFFFu;
    if (address >= kRamBase) {
        ram_[address - kRamBase] = value;
        return;
    }
    throw std::out_of_range("Unmapped Mega Drive write");
}

void MemoryBus::write_ram(std::uint32_t address, std::span<const std::uint8_t> data) {
    address &= 0x00FFFFFFu;
    if (address < kRamBase || (address - kRamBase) + data.size() > ram_.size()) {
        throw std::out_of_range("RAM block write out of range");
    }
    std::copy(data.begin(), data.end(), ram_.begin() + static_cast<std::ptrdiff_t>(address - kRamBase));
}

} // namespace oasis::genesis
