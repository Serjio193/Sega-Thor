#include "game/resource_diagnostic.hpp"

#include "game/genesis_graphics.hpp"
#include "game/render/framebuffer.hpp"
#include "game/resource_loader.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace oasis::game::render {
namespace {

constexpr std::uint32_t kBackground = 0x00101828U;
constexpr std::array<std::uint32_t, 16> kDiagnosticPalette{
    0x00000000U, 0x00202030U, 0x00405070U, 0x007090A0U,
    0x00B0D0D0U, 0x00E0E0C0U, 0x00D0A060U, 0x00B06040U,
    0x00803050U, 0x00602080U, 0x003040A0U, 0x004080D0U,
    0x0060C080U, 0x00A0D060U, 0x00D0D040U, 0x00F0F0F0U};

void fill_rect(std::span<std::uint32_t> pixels, std::uint32_t color,
               std::size_t x, std::size_t y, std::size_t width,
               std::size_t height) noexcept {
    constexpr auto framebuffer_width = SoftwareFramebuffer::kWidth;
    constexpr auto framebuffer_height = SoftwareFramebuffer::kHeight;
    const auto x_end = std::min<std::size_t>(x + width, framebuffer_width);
    const auto y_end = std::min<std::size_t>(y + height, framebuffer_height);
    for (auto row = y; row < y_end; ++row) {
        for (auto column = x; column < x_end; ++column) {
            pixels[row * framebuffer_width + column] = color;
        }
    }
}

} // namespace

void ResourceDiagnosticScreen::transfer_to_vram(
    genesis::Vdp& vdp, std::span<const std::uint8_t> resource) const {
    if (resource.size() != kTileBytes * kTileCount) {
        throw std::invalid_argument("diagnostic resource must contain 128 tiles");
    }
    vdp.write_vram(kResourceVramAddress, resource);
}

void ResourceDiagnosticScreen::render(const genesis::Vdp& vdp,
                                      std::span<std::uint32_t> destination) const noexcept {
    constexpr auto width = static_cast<std::size_t>(SoftwareFramebuffer::kWidth);
    constexpr auto height = static_cast<std::size_t>(SoftwareFramebuffer::kHeight);
    if (destination.size() < width * height) return;
    destination = destination.first(width * height);
    std::fill(destination.begin(), destination.end(), kBackground);

    const auto& vram = vdp.vram();
    constexpr std::size_t origin_x = 32U;
    constexpr std::size_t origin_y = 24U;
    constexpr std::size_t scale = 2U;
    for (std::size_t tile = 0; tile < kTileCount; ++tile) {
        const auto pixels = decode_genesis_4bpp_tile(
            std::span<const std::uint8_t>(vram).subspan(kResourceVramAddress + tile * kTileBytes,
                                                        kTileBytes));
        const auto tile_x = origin_x + (tile % 16U) * 8U * scale;
        const auto tile_y = origin_y + (tile / 16U) * 8U * scale;
        for (std::size_t y = 0; y < 8U; ++y) {
            for (std::size_t x = 0; x < 8U; ++x) {
                fill_rect(destination, kDiagnosticPalette[pixels[y * 8U + x]],
                          tile_x + x * scale, tile_y + y * scale, scale, scale);
            }
        }
    }
}

} // namespace oasis::game::render
