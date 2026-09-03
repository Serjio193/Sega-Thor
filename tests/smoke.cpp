#include "core/rom.hpp"
#include "game/translated_routines.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>

int main() {
    const auto path = std::filesystem::temp_directory_path() / "oasis_smoke.bin";
    {
        std::array<std::uint8_t, 0x400> data{};
        std::ofstream out(path, std::ios::binary);
        out.write(reinterpret_cast<const char*>(data.data()), data.size());
    }

    const auto rom = oasis::Rom::load(path);
    oasis::genesis::MemoryBus bus(rom);

    const std::array<std::uint8_t, 4> sample{1, 2, 3, 4};
    oasis::game::tilecopy_to_ram(bus, sample, oasis::genesis::MemoryBus::kRamBase + 0x10);
    oasis::game::tilecopy_to_vram(bus, sample, 0x20);

    assert(bus.read8(oasis::genesis::MemoryBus::kRamBase + 0x10) == 1);
    assert(bus.vdp().vram()[0x20] == 1);
    assert(oasis::game::kDecompressGraphicsAddress == 0x3820);

    std::filesystem::remove(path);
    return 0;
}
