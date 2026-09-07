#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "game/genesis_graphics.hpp"
#include "game/resource_diagnostic.hpp"
#include "game/resource_loader.hpp"
#include "game/render/framebuffer.hpp"
#include "genesis/vdp.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <exception>
#include <iostream>
#include <span>
#include <vector>

namespace {

template <typename Function>
void expect_failure(Function&& function) {
    bool failed = false;
    try {
        function();
    } catch (const std::exception&) {
        failed = true;
    }
    assert(failed);
}

std::span<const std::uint8_t> framebuffer_bytes(
    std::span<const std::uint32_t> pixels) {
    return {reinterpret_cast<const std::uint8_t*>(pixels.data()), pixels.size_bytes()};
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    const auto rom = oasis::Rom::load(argv[1]);
    const auto resource = oasis::game::load_verified_resource(
        rom.bytes(), oasis::game::kVerifiedResourceId);
    assert(resource.table_entry == oasis::game::kResourceTableEntry);
    assert(resource.rom_input_start == oasis::game::kResourceInputAddress);
    assert(resource.compressed_size == oasis::game::kResourceCompressedSize);
    assert(resource.bytes.size() == oasis::game::kResourceOutputSize);
    assert(oasis::calculate_sha256(resource.bytes) == oasis::game::kResourceOutputSha256);

    oasis::genesis::Vdp vdp;
    oasis::game::render::ResourceDiagnosticScreen screen;
    screen.transfer_to_vram(vdp, resource.bytes);
    const auto vram = std::span<const std::uint8_t>(vdp.vram()).subspan(
        oasis::game::kResourceVramAddress, oasis::game::kResourceOutputSize);
    assert(std::equal(resource.bytes.begin(), resource.bytes.end(), vram.begin()));
    assert(oasis::calculate_sha256(vram) == oasis::game::kResourceOutputSha256);

    for (std::size_t tile = 0; tile < screen.kTileCount; ++tile) {
        const auto decoded = oasis::game::decode_genesis_4bpp_tile(
            vram.subspan(tile * screen.kTileBytes, screen.kTileBytes));
        assert(decoded.size() == 64U);
    }

    std::vector<std::uint32_t> pixels(
        oasis::game::render::SoftwareFramebuffer::kWidth *
        oasis::game::render::SoftwareFramebuffer::kHeight);
    std::vector<std::uint32_t> repeated(pixels.size());
    screen.render(vdp, pixels);
    screen.render(vdp, repeated);
    assert(pixels == repeated);
    const auto framebuffer_hash = oasis::calculate_sha256(framebuffer_bytes(pixels));
    assert(framebuffer_hash ==
           "d2b7655501ff3babf6ef9dd44af720b3afab3ba3e2673fa7aeff33248a3ab1b1");

    auto wrong_rom = rom.bytes();
    wrong_rom[0x200] ^= 0x01U;
    expect_failure([&] { static_cast<void>(oasis::game::load_verified_resource(wrong_rom, 3)); });
    expect_failure([&] { static_cast<void>(oasis::game::load_verified_resource(rom.bytes(), 2)); });
    expect_failure([&] {
        const auto truncated = std::span<const std::uint8_t>(rom.bytes()).first(0x1000U);
        static_cast<void>(oasis::game::load_verified_resource(truncated, 3));
    });
    expect_failure([&] {
        oasis::game::validate_resource_id3_output(
            oasis::game::kResourceCompressedSize,
            std::span<const std::uint8_t>(resource.bytes).first(0x0FFFU));
    });
    expect_failure([&] {
        vdp.write_vram(oasis::genesis::Vdp::kVramSize - 1U, resource.bytes);
    });
    expect_failure([&] {
        std::vector<std::uint8_t> substituted(resource.bytes.size(), 0xA5U);
        oasis::game::validate_resource_id3_output(
            oasis::game::kResourceCompressedSize, substituted);
    });

    std::cout << "resource_id=3 compressed=0x" << std::hex
              << resource.compressed_size << std::dec
              << " output_sha256=" << oasis::game::kResourceOutputSha256
              << " vram_sha256=" << oasis::calculate_sha256(vram)
              << " tile_count=" << screen.kTileCount
              << " framebuffer_sha256=" << framebuffer_hash << '\n';
    return 0;
}
