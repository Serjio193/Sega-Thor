#include "genesis/vdp.hpp"

#include <algorithm>
#include <stdexcept>

namespace oasis::genesis {
namespace {

template <std::size_t N>
void write_bytes(std::array<std::uint8_t, N>& target,
                 std::size_t address,
                 std::span<const std::uint8_t> data,
                 const char* message) {
    if (address > target.size() || data.size() > target.size() - address) {
        throw std::out_of_range(message);
    }
    std::copy(data.begin(), data.end(), target.begin() + static_cast<std::ptrdiff_t>(address));
}

template <std::size_t N>
std::uint16_t read_word(const std::array<std::uint8_t, N>& source,
                        std::size_t address,
                        const char* message) {
    if (address > source.size() || source.size() - address < 2) {
        throw std::out_of_range(message);
    }
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(source[address]) << 8U) |
        static_cast<std::uint16_t>(source[address + 1]));
}

} // namespace

void Vdp::write_vram(std::size_t address, std::span<const std::uint8_t> data) {
    write_bytes(vram_, address, data, "VDP VRAM write out of range");
}

void Vdp::write_cram(std::size_t address, std::span<const std::uint8_t> data) {
    write_bytes(cram_, address, data, "VDP CRAM write out of range");
}

void Vdp::write_vsram(std::size_t address, std::span<const std::uint8_t> data) {
    write_bytes(vsram_, address, data, "VDP VSRAM write out of range");
}

std::uint16_t Vdp::read_vram_word(std::size_t address) const {
    return read_word(vram_, address, "VDP VRAM read out of range");
}

std::uint16_t Vdp::read_cram_word(std::size_t address) const {
    return read_word(cram_, address, "VDP CRAM read out of range");
}

std::uint16_t Vdp::read_vsram_word(std::size_t address) const {
    return read_word(vsram_, address, "VDP VSRAM read out of range");
}

} // namespace oasis::genesis
