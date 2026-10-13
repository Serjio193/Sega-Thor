#include "tools/hybrid/native_island_runtime.hpp"

#include <array>
#include <cstdint>

namespace {
std::array<std::uint8_t, 0x100> ram{};
std::array<unsigned, 18> registers{};
unsigned writes{};

int peek(unsigned address) {
    if (address < 0xFF1000U || address >= 0xFF1100U) return -1;
    return ram[address - 0xFF1000U];
}
bool write(unsigned address, int width, unsigned value) {
    if ((width != 1 && width != 2 && width != 4) ||
        address < 0xFF1000U || address + static_cast<unsigned>(width) > 0xFF1100U)
        return false;
    ++writes;
    for (int i = 0; i < width; ++i)
        ram[address - 0xFF1000U + static_cast<unsigned>(i)] =
            static_cast<std::uint8_t>(value >> (8 * (width - i - 1)));
    return true;
}
void set_register(unsigned index, unsigned value) {
    if (index < registers.size()) registers[index] = value;
}
void put_word(unsigned address, std::uint16_t value) {
    ram[address - 0xFF1000U] = static_cast<std::uint8_t>(value >> 8U);
    ram[address - 0xFF1000U + 1U] = static_cast<std::uint8_t>(value);
}
} // namespace

int main() {
    using namespace oasis::hybrid::native_island;
    if (!supported_helper("9d70") || supported_helper("b730")) return 1;
    CpuState cpu;
    cpu.d[0] = 0x20U;
    cpu.d[1] = 0x30U;
    cpu.d[3] = 0xAABB7777U;
    cpu.a[6] = 0xFF1000U;
    cpu.a[7] = 0xFF20F0U;
    cpu.sr = 0x2000U;
    put_word(0xFF1046U, 8U);
    ram[0x40] = 0x05U;
    const Access access{peek, write, set_register};
    Counters counters;

    if (route_9d70(false, true, true, 0x9D70U, 0x9144U, 0x61000C2AU,
                   0x9148U, "M68K_009D70_CALLER_009144_V1", cpu, access,
                   counters) != Result::RomFallback || writes != 0U)
        return 2;
    if (route_9d70(true, true, true, 0x9D70U, 0x9146U, 0x61000C28U,
                   0x914AU, "M68K_009D70_CALLER_009144_V1", cpu, access,
                   counters) != Result::RomFallback || writes != 0U)
        return 3;
    if (route_9d70(true, true, false, 0x9D70U, 0x9144U, 0x61000C2AU,
                   0x9148U, "M68K_009D70_CALLER_009144_V1", cpu, access,
                   counters) != Result::RomFallback || writes != 0U)
        return 4;
    if (route_9d70(true, true, true, 0x9D70U, 0x9144U, 0x61000C2AU,
                   0x9148U, "M68K_009D70_CALLER_009144_V0", cpu, access,
                   counters) != Result::RomFallback || writes != 0U)
        return 5;
    auto invalid_cpu = cpu;
    invalid_cpu.a[6] = 0x00001000U;
    if (route_9d70(true, true, true, 0x9D70U, 0x9144U, 0x61000C2AU,
                   0x9148U, "M68K_009D70_CALLER_009144_V1", invalid_cpu,
                   access, counters) != Result::RomFallback || writes != 0U)
        return 6;
    if (counters.contract_rejects != 2U || counters.precondition_rejects != 1U ||
        counters.rom_fallbacks != 5U)
        return 7;
    counters = {};
    if (route_9d70(true, true, true, 0x9D70U, 0x9144U, 0x61000C2AU,
                   0x9148U, "M68K_009D70_CALLER_009144_V1", cpu, access,
                   counters) != Result::NativeRouted)
        return 8;
    if (registers[0] != 0x20U) return 9;
    if (registers[3] != 0xAABB0028U) return 10;
    if (registers[16] != 0x9D86U) return 11;
    if (registers[17] != 0x2009U) return 12;
    if (writes != 0U || counters.native_calls != 1U) return 13;

    counters = {};
    cpu.d[0] = 1U;
    cpu.d[1] = 8U;
    put_word(0xFF1046U, 8U);
    ram[0x40] = 0x02U;
    if (route_9d70(true, true, true, 0x9D70U, 0x9144U, 0x61000C2AU,
                   0x9148U, "M68K_009D70_CALLER_009144_V1", cpu, access,
                   counters) != Result::NativeRouted)
        return 14;
    if (ram[0x40] != 0x4AU || registers[16] != 0x9D90U || writes != 1U)
        return 15;
    return 0;
}
