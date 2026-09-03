#pragma once

#include "core/rom.hpp"
#include "genesis/vdp.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::genesis {

class MemoryBus {
public:
    static constexpr std::uint32_t kVdpData = 0xC00000;
    static constexpr std::uint32_t kVdpCtrl = 0xC00004;
    static constexpr std::uint32_t kRamBase = 0xFF0000;
    static constexpr std::size_t kRamSize = 64 * 1024;

    explicit MemoryBus(const oasis::Rom& rom) : rom_(rom) {}

    [[nodiscard]] std::uint8_t read8(std::uint32_t address) const;
    void write8(std::uint32_t address, std::uint8_t value);
    void write_ram(std::uint32_t address, std::span<const std::uint8_t> data);

    [[nodiscard]] Vdp& vdp() noexcept { return vdp_; }
    [[nodiscard]] const Vdp& vdp() const noexcept { return vdp_; }

private:
    const oasis::Rom& rom_;
    std::array<std::uint8_t, kRamSize> ram_{};
    Vdp vdp_{};
};

} // namespace oasis::genesis
