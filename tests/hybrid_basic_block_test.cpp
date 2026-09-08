#include "tools/hybrid/basic_block.hpp"

#include <cassert>
#include <cstdint>
#include <sstream>
#include <unordered_map>

namespace {
struct Fake {
    std::uint32_t regs[18]{};
    std::uint32_t cycles{100};
    std::uint32_t refresh{1000};
    std::uint32_t ir{};
    std::uint32_t pref_addr{};
    std::uint32_t pref_data{};
    std::unordered_map<unsigned, std::uint8_t> memory;
    static Fake* current;

    static unsigned reg(unsigned index) { return current->regs[index]; }
    static void set_reg(unsigned index, unsigned value) { current->regs[index] = value; }
    static int peek(unsigned address) {
        const auto it = current->memory.find(address);
        return it == current->memory.end() ? 0 : it->second;
    }
    static unsigned read(unsigned address, int width) {
        unsigned value = 0;
        for (int i = 0; i < width; ++i) value = (value << 8U) | peek(address + i);
        return value;
    }
    static unsigned instruction_cycles(unsigned opcode) { return opcode == 0x4A79U ? 34U : 0U; }
    static unsigned field(unsigned index) {
        if (index == 18U) return current->ir;
        if (index == 19U) return current->pref_addr;
        if (index == 20U) return current->pref_data;
        if (index == 21U) return current->cycles;
        if (index == 22U) return current->refresh;
        return 0;
    }
    static unsigned period() { return 896; }
    static unsigned penalty() { return 14; }
};

Fake* Fake::current = nullptr;

oasis::hybrid::BasicBlockApi api() {
    return {Fake::reg, Fake::set_reg, Fake::peek, nullptr, Fake::read, nullptr,
            nullptr, nullptr, nullptr, nullptr, Fake::instruction_cycles, Fake::field,
            Fake::period, Fake::penalty};
}
} // namespace

int main() {
    Fake fake;
    Fake::current = &fake;
    fake.regs[16] = 0x3A85EU;
    fake.memory[0x3A85E] = 0x4A;
    fake.memory[0x3A85F] = 0x79;
    fake.memory[0x3A860] = 0x00;
    fake.memory[0x3A861] = 0xFF;
    fake.memory[0x3A862] = 0x16;
    fake.memory[0x3A863] = 0x54;
    fake.memory[0x3A864] = 0x4E;
    fake.memory[0x3A865] = 0x75;
    std::ostringstream log;
    oasis::hybrid::BasicBlockRegistry registry(api(), oasis::hybrid::BasicBlockMode::SHADOW_NATIVE, log);
    assert(registry.dispatch(0x3A85EU) == 0);
    fake.regs[16] = 0x3A864U;
    fake.regs[17] = 4U;
    fake.ir = 0x4A79U;
    fake.pref_addr = 0x3A864U;
    fake.pref_data = 0x4E75U;
    fake.cycles = 134U;
    registry.event(2, 2, 0xFF1654U, 0);
    registry.event(1 << 14, 0, 0x3A864U, 0);
    assert(registry.complete());
    assert(registry.metrics().shadow_comparisons == 1U);
    return 0;
}
