#include "tools/hybrid/dispatch.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
using namespace oasis::hybrid;
State cpu;
std::vector<std::uint8_t> rom(0x4000), ram(65536);
unsigned reg(unsigned i) { return cpu[i]; }
int peek(unsigned address) {
    if (address < rom.size()) return rom[address];
    if (address >= 0xFF0000 && address < 0x1000000) return ram[address & 0xFFFF];
    return -1;
}
void check(bool value) { if (!value) throw std::runtime_error("dispatch regression"); }
void reset() {
    cpu.fill(0);
    for (unsigned i = 0; i < 16; ++i) cpu[i] = 0x12345600 + i;
    cpu[8] = 0x1000; cpu[9] = 0xFF1000; cpu[15] = 0xFFFF00;
    cpu[16] = target; cpu[17] = 0x2100;
    std::fill(ram.begin(), ram.end(), 0xCC);
    const std::array<std::uint8_t, 7> source{6, 0, 3, 0xA5, 0x5A, 0x11, 0};
    std::copy(source.begin(), source.end(), rom.begin() + 0x1000);
    ram[0xFF00] = 0; ram[0xFF01] = 0; ram[0xFF02] = 8; ram[0xFF03] = 0;
}
void write(Dispatch& dispatch, unsigned address, unsigned value, unsigned width) {
    dispatch.hook(4, width, address, value);
    for (unsigned i = 0; i < width; ++i)
        ram[(address + i) & 0xFFFF] = value >> ((width - i - 1) * 8);
}
void run(Dispatch& dispatch, bool corrupt_output = false, bool corrupt_register = false) {
    dispatch.hook(1, 0, target, 0);
    // Independent explicit MOVEM bus-word oracle for D0/D1/D2/A2.
    for (const auto [address, value] : std::array<std::pair<unsigned, unsigned>, 8>{{
        {0xFFFEFE, 0x560A}, {0xFFFEFC, 0x1234}, {0xFFFEFA, 0x5602}, {0xFFFEF8, 0x1234},
        {0xFFFEF6, 0x5601}, {0xFFFEF4, 0x1234}, {0xFFFEF2, 0x5600}, {0xFFFEF0, 0x1234}}})
        write(dispatch, address, value, 2);
    write(dispatch, 0xFF1000, 0xA5, 1);
    write(dispatch, 0xFF1001, corrupt_output ? 0xFF : 0x5A, 1);
    write(dispatch, 0xFF1002, 0x11, 1);
    cpu[8] = 0x1007; cpu[9] = 0xFF1003; cpu[15] = 0xFFFF04;
    cpu[16] = 0x800; cpu[17] = 0x2104;
    if (corrupt_register) cpu[4] ^= 1;
    dispatch.hook(1, 0, 0x800, 0);
}
}
int main() {
    try {
        std::ostringstream log;
        reset();
        Dispatch clean({reg, peek}, Mode::SHADOW_NATIVE, rom, log);
        run(clean);
        check(clean.complete() && clean.comparisons == 1 && clean.divergences == 0);
        for (unsigned variant = 0; variant < 2; ++variant) {
            reset();
            Dispatch bad({reg, peek}, Mode::SHADOW_NATIVE, rom, log);
            run(bad, variant == 0, variant == 1);
            check(!bad.complete() && bad.divergences == 1 && bad.comparisons == 0);
            check(bad.error().find("FIRST_DIVERGENCE") != std::string::npos);
        }
        reset();
        Dispatch extra({reg, peek}, Mode::SHADOW_NATIVE, rom, log);
        extra.hook(1, 0, target, 0);
        extra.hook(4, 1, 0xFF2000, 42);
        check(extra.divergences == 1);
        reset();
        Dispatch irq({reg, peek}, Mode::SHADOW_NATIVE, rom, log);
        irq.hook(1, 0, target, 0);
        cpu[16] = 0x3824;
        irq.hook(2, 4, 0x78, 0x900);
        irq.hook(1, 0, 0x900, 0);
        irq.hook(4, 1, 0xFF2000, 42); // Unrelated external ISR write is allowed.
        cpu[16] = 0x3824;
        irq.hook(1, 0, 0x3824, 0);
        check(irq.interrupts == 1 && irq.divergences == 0 && !irq.complete());
        irq.hook(2, 4, 0x78, 0x900);
        irq.hook(4, 1, 0xFF1000, 42); // ISR may not alter the captured footprint.
        check(irq.divergences == 1);
        reset();
        Dispatch emulated({reg, peek}, Mode::EMULATED, rom, log);
        emulated.hook(1, 0, target, 0);
        check(emulated.calls == 1 && emulated.comparisons == 0 && emulated.complete());
        std::cout << "synthetic shadow, corruption, IRQ isolation and emulated dispatch verified\n";
    } catch (const std::exception& error) { std::cerr << error.what(); return 1; }
}
