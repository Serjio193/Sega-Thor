#pragma once

#include "genesis/vdp.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::game::render {

class ResourceDiagnosticScreen final {
public:
    static constexpr std::size_t kTileBytes = 32U;
    static constexpr std::size_t kTileCount = 0x1000U / kTileBytes;

    void transfer_to_vram(genesis::Vdp& vdp,
                          std::span<const std::uint8_t> resource) const;
    void render(const genesis::Vdp& vdp, std::span<std::uint32_t> destination) const noexcept;
};

} // namespace oasis::game::render
