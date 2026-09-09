#include "tools/hybrid/candidate_604bc.hpp"

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
    std::vector<unsigned> fetched, began, finished, returned;
    unsigned boundary_count{}, boundary_after{};
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
    static unsigned fetch16() {
        const auto pc = current->regs[16];
        const auto value = (peek(pc) << 8) | peek(pc + 1);
        current->regs[16] += 2;
        current->fetched.push_back(static_cast<unsigned>(value));
        return static_cast<unsigned>(value);
    }
    static void begin_instruction(unsigned opcode) { current->began.push_back(opcode); }
    static void finish_instruction(unsigned opcode) { current->finished.push_back(opcode); }
    static void set_return_state(unsigned pc, unsigned, unsigned) {
        current->regs[16] = pc;
        current->returned.push_back(pc);
    }
    static unsigned boundary_reason() {
        ++current->boundary_count;
        if (current->boundary_after != 0 && current->boundary_count == current->boundary_after) {
            return 2;
        }
        return 0;
    }
    void word(unsigned address, unsigned value) { poke(address, 2, value); }
    void longword(unsigned address, unsigned value) { poke(address, 4, value); }
    void reset() {
        regs.fill(0);
        memory.clear();
        fetched.clear();
        began.clear();
        finished.clear();
        returned.clear();
        boundary_count = 0;
        boundary_after = 0;
        regs[13] = 0x00FF2000;
        regs[15] = 0x00FFFF00;
        regs[16] = 0x0604BC;
        regs[17] = 0x2711;
        word(0x0604BC, 0x4DF9); word(0x0604BE, 0x00FF); word(0x0604C0, 0x0628);
        word(0x0604C2, 0x08EE); word(0x0604C4, 0x0004);
        word(0x0604C8, 0x4DF9); word(0x0604CA, 0x00FF); word(0x0604CC, 0x06F2);
        word(0x0604CE, 0x08EE); word(0x0604D0, 0x0004);
        word(0x0604D4, 0x41ED); word(0x0604D6, 0x0005);
        word(0x0604D8, 0x51D8); word(0x0604DA, 0x51D8); word(0x0604DC, 0x51D8);
        word(0x0604DE, 0x51F9); word(0x0604E0, 0x00FF); word(0x0604E2, 0x0016);
        word(0x0604E4, 0x4E75);
        memory[0xFF0628] = 0;
        memory[0xFF06F2] = 0x10;
        longword(0xFFFF00, 0x00000200);
    }
};
Fake* Fake::current = nullptr;
}

int main() {
    Fake fake;
    Fake::current = &fake;
    fake.reset();
    std::ostringstream log;
    oasis::hybrid::Candidate604BC candidate(
        {Fake::reg, Fake::set_reg, Fake::peek, Fake::poke, Fake::set_return_state,
         {}, {}, {}, {}, Fake::fetch16, Fake::begin_instruction, Fake::finish_instruction},
        oasis::hybrid::Mode::NATIVE_OVERRIDE, log);
    candidate.hook(1, 2, 0x0604BC, 0);
    assert(candidate.complete());
    assert(candidate.calls == 1 && candidate.override_calls == 1 &&
           candidate.native_instructions == 10 && candidate.divergences == 0);
    assert((fake.began == std::vector<unsigned>{
        0x4DF9, 0x08EE, 0x4DF9, 0x08EE, 0x41ED,
        0x51D8, 0x51D8, 0x51D8, 0x51F9, 0x4E75}));
    assert(fake.began == fake.finished && fake.returned.size() == 11);
    assert(fake.regs[8] == 0x00FF2008 && fake.regs[14] == 0x00FF06F2 &&
           fake.regs[15] == 0x00FFFF04 && fake.regs[16] == 0x200);
    assert(fake.memory[0xFF0628] == 0x10 && fake.memory[0xFF06F2] == 0x10);
    assert(fake.memory[0xFF2005] == 0 && fake.memory[0xFF2006] == 0 &&
           fake.memory[0xFF2007] == 0 && fake.memory[0xFF0016] == 0);
    assert((fake.fetched == std::vector<unsigned>{
        0x4DF9, 0x00FF, 0x0628, 0x08EE, 0x0004, 0x4DF9, 0x00FF,
        0x06F2, 0x08EE, 0x0004, 0x41ED, 0x0005, 0x51D8, 0x51D8,
        0x51D8, 0x51F9, 0x00FF, 0x0016, 0x4E75}));

    fake.reset();
    fake.boundary_after = 6;
    std::ostringstream resumed_log;
    oasis::hybrid::Candidate604BC resumed(
        {Fake::reg, Fake::set_reg, Fake::peek, Fake::poke, Fake::set_return_state,
         {}, {}, {}, {}, Fake::fetch16, Fake::begin_instruction, Fake::finish_instruction,
         Fake::boundary_reason},
        oasis::hybrid::Mode::NATIVE_OVERRIDE, resumed_log);
    assert(resumed.dispatch(0x0604BC) == 2);
    assert(!resumed.complete() && resumed.native_boundary_yields == 1);
    assert(resumed.dispatch(0x0604DA) == 1);
    assert(resumed.complete() && resumed.native_resumptions == 1 &&
           resumed.native_instructions == 10 && resumed.override_calls == 1);
}
