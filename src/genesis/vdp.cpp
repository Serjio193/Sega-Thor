#include "genesis/vdp.hpp"

#include <algorithm>
#include <stdexcept>

namespace oasis::genesis {

void Vdp::write_vram(std::size_t address, std::span<const std::uint8_t> data) {
    if (address + data.size() > vram_.size()) {
        throw std::out_of_range("VDP VRAM write out of range");
    }
    std::copy(data.begin(), data.end(), vram_.begin() + static_cast<std::ptrdiff_t>(address));
}

} // namespace oasis::genesis
