#include "tools/hybrid/candidate_2d66.hpp"
#include <array>
#include <cassert>
#include <cstdint>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace {
struct Fake {
    std::array<unsigned, 18> regs{};
    std::unordered_map<unsigned, std::uint8_t> memory;
    static Fake* current;

    static unsigned reg(unsigned index) { return current->regs[index]; }
    static void set_reg(unsigned index, unsigned value) { current->regs[index] = value; }
    static int peek(unsigned address) {
        const auto it = current->memory.find(address);
        return it == current->memory.end() ? 0 : it->second;
    }
    static void poke(unsigned address, int width, unsigned value) {
        for (int i = 0; i < width; ++i)
            current->memory[address + static_cast<unsigned>(i)] =
                static_cast<std::uint8_t>(value >> (8 * (width - i - 1)));
    }
    void word(unsigned address, unsigned value) { poke(address, 2, value); }
    void longword(unsigned address, unsigned value) { poke(address, 4, value); }
    void reset() {
        regs.fill(0);
        memory.clear();
        regs[7] = 0x11223344;
        regs[11] = 0x00FF2000;
        regs[14] = 0x100;
        regs[15] = 0xFFFF00;
        regs[16] = 0x2D66;
        regs[17] = 0x2713;
        word(0x100, 0x0201);
        word(0x102, 0x1234);
        word(0x104, 0x8000);
        longword(0xFFFF00, 0x00000200);
    }
};
Fake* Fake::current = nullptr;

void original_body(Fake& fake) {
    fake.word(0xFFFEFE, 0x2000);
    fake.word(0xFFFEFC, 0x00FF);
    fake.word(0xFFFEFA, 0x3344);
    fake.word(0xFFFEF8, 0x1122);
    fake.word(0xFF134E, 0x1234);
    fake.word(0xFF1350, 0x8000);
    fake.regs[14] += 6;
    fake.regs[15] += 4;
    fake.regs[16] = 0x200;
    fake.regs[17] = 0x2718;
}

void run_shadow() {
    Fake fake;
    Fake::current = &fake;
    fake.reset();
    std::vector<std::uint8_t> rom(0x400);
    rom[0x100] = 2; rom[0x101] = 1; rom[0x102] = 0x12; rom[0x103] = 0x34;
    rom[0x104] = 0x80; rom[0x105] = 0x00;
    std::ostringstream log;
    oasis::hybrid::Candidate2D66 candidate(
        {Fake::reg, Fake::set_reg, Fake::peek, Fake::poke},
        oasis::hybrid::Mode::SHADOW_NATIVE, rom, log);
    candidate.hook(1, 2, 0x2D66, 0);
    original_body(fake);
    candidate.hook(4, 2, 0xFFFEFE, 0x2000);
    candidate.hook(4, 2, 0xFFFEFC, 0x00FF);
    candidate.hook(4, 2, 0xFFFEFA, 0x3344);
    candidate.hook(4, 2, 0xFFFEF8, 0x1122);
    candidate.hook(4, 2, 0xFF134E, 0x1234);
    candidate.hook(4, 2, 0xFF1350, 0x8000);
    candidate.hook(1, 2, 0x200, 0);
    assert(candidate.complete());
    assert(candidate.calls == 1 && candidate.comparisons == 1 && candidate.divergences == 0);
}

void run_override() {
    Fake fake;
    Fake::current = &fake;
    fake.reset();
    std::vector<std::uint8_t> rom(0x400);
    rom[0x100] = 2; rom[0x101] = 1; rom[0x102] = 0x12; rom[0x103] = 0x34;
    rom[0x104] = 0x80; rom[0x105] = 0x00;
    std::ostringstream log;
    oasis::hybrid::Candidate2D66 candidate(
        {Fake::reg, Fake::set_reg, Fake::peek, Fake::poke},
        oasis::hybrid::Mode::NATIVE_OVERRIDE, rom, log);
    candidate.hook(1, 2, 0x2D66, 0);
    assert(candidate.complete());
    assert(candidate.calls == 1 && candidate.override_calls == 1 && candidate.divergences == 0);
    assert(fake.regs[14] == 0x106 && fake.regs[15] == 0xFFFF04 && fake.regs[16] == 0x200);
    assert(Fake::peek(0xFF134E) == 0x12 && Fake::peek(0xFF134F) == 0x34);
    assert(Fake::peek(0xFF1350) == 0x80 && Fake::peek(0xFF1351) == 0x00);
}
}

int main() {
    run_shadow();
    run_override();
    return 0;
}
