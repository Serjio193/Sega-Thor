#include "tools/hybrid/generated_blocks.hpp"

#include <cassert>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace {

struct Fake {
    std::uint32_t regs[18]{};
    std::unordered_map<std::uint32_t, std::uint8_t> memory;
    unsigned begins{};
    unsigned extra_cycles{};
    unsigned refresh_skips{};
    unsigned fetch_index{};
    unsigned boundary_calls{};
    unsigned stop_after{};
    std::vector<unsigned> fetch_words;
    static Fake* current;

    static unsigned reg(unsigned index) { return current->regs[index]; }
    static void set_reg(unsigned index, unsigned value) { current->regs[index] = value; }
    static unsigned fetch16() {
        current->regs[16] += 2U;
        return current->fetch_words.at(current->fetch_index++);
    }
    static int peek(unsigned address) {
        const auto it = current->memory.find(address);
        return it == current->memory.end() ? 0 : it->second;
    }
    static unsigned read(unsigned address, int width) {
        unsigned value = 0;
        for (int i = 0; i < width; ++i) value = (value << 8U) | peek(address + i);
        return value;
    }
    static void write(unsigned address, int width, unsigned value) {
        for (int i = 0; i < width; ++i)
            current->memory[address + static_cast<unsigned>(i)] =
                static_cast<std::uint8_t>(value >> (8 * (width - i - 1)));
    }
    static void begin(unsigned) { ++current->begins; }
    static void finish(unsigned) {}
    static void add_cycles(int value) { current->extra_cycles += value; }
    static void skip_refresh() { ++current->refresh_skips; }
    static oasis::hybrid::BlockExitReason boundary_reason() {
        ++current->boundary_calls;
        return current->stop_after && current->boundary_calls >= current->stop_after
                   ? oasis::hybrid::BlockExitReason::EVENT_BOUNDARY
                   : oasis::hybrid::BlockExitReason::CONTINUE_BLOCK;
    }
};

Fake* Fake::current = nullptr;

oasis::hybrid::BasicBlockApi api() {
    return {Fake::reg, Fake::set_reg, Fake::peek, Fake::fetch16, Fake::read, Fake::write,
            Fake::begin, Fake::finish, Fake::add_cycles, Fake::skip_refresh, nullptr,
            nullptr, nullptr, nullptr, Fake::boundary_reason};
}

} // namespace

int main() {
    Fake fake;
    Fake::current = &fake;
    fake.regs[7] = 0x11223344;
    fake.regs[11] = 0x00FF2000;
    fake.regs[14] = 0x100;
    fake.regs[8] = 0x100;
    fake.regs[15] = 0xFFFF00;
    fake.regs[17] = 0x2713;
    fake.memory[0x100] = 0x02;
    fake.memory[0x101] = 0x01;
    fake.memory[0x102] = 0x12;
    fake.memory[0x103] = 0x34;
    fake.memory[0x104] = 0x80;
    fake.memory[0x105] = 0x00;
    fake.fetch_words = {0x48E7, 0x0110, 0x4247, 0x1E1E, 0x47F9,
                        0x00FF, 0x134C, 0xD6C7, 0x1E1E, 0x36DE};

    auto bridge = api();
    const auto exit = oasis::hybrid::generated::execute_0x002D66(bridge, 0x2D66U);
    assert(exit.reason == oasis::hybrid::BlockExitReason::NORMAL_EXIT);
    assert(exit.instructions_executed == 7U);
    assert(fake.begins == 7U);
    assert(fake.fetch_index == fake.fetch_words.size());
    assert(fake.extra_cycles == 112U && fake.refresh_skips == 1U);
    assert(fake.regs[7] == 0x11220001U);
    assert(fake.regs[11] == 0x00FF1350U);
    assert(fake.regs[14] == 0x104U);
    assert(fake.regs[15] == 0xFFFEF8U);
    assert(fake.regs[17] == 0x2710U);
    assert(Fake::read(0xFFFEF8, 2) == 0x1122U);
    assert(Fake::read(0xFFFEFA, 2) == 0x3344U);
    assert(Fake::read(0xFFFEFC, 2) == 0x00FFU);
    assert(Fake::read(0xFFFEFE, 2) == 0x2000U);
    assert(Fake::read(0xFF134E, 2) == 0x1234U);
    assert(Fake::read(0xFF1350, 2) == 0U);

    fake.fetch_index = 0;
    fake.fetch_words = {0x4A79, 0x00FF, 0x1658};
    fake.regs[16] = 0x0032EEU;
    fake.regs[17] = 0;
    fake.stop_after = 1;
    fake.boundary_calls = 0;
    const auto yielded = oasis::hybrid::generated::execute_0x0032EE(bridge, 0x0032EEU);
    assert(yielded.reason == oasis::hybrid::BlockExitReason::EVENT_BOUNDARY);
    assert(yielded.instructions_executed == 1U && yielded.next_pc == 0x0032F4U);
    assert(fake.boundary_calls == 1U);

    fake.fetch_index = 0;
    fake.fetch_words = {0x66F8};
    fake.regs[17] = 0;
    fake.stop_after = 0;
    fake.boundary_calls = 0;
    const auto resumed = oasis::hybrid::generated::execute_0x0032EE(bridge, 0x0032F4U);
    assert(resumed.reason == oasis::hybrid::BlockExitReason::NORMAL_EXIT);
    assert(resumed.instructions_executed == 1U && resumed.next_pc == 0x0032EEU);

    fake.fetch_index = 0;
    fake.fetch_words = {0xD058, 0xD058, 0xD058, 0xD058, 0xD058, 0xD058,
                        0xD058, 0xD058, 0xD058, 0xD058, 0xD058, 0xD058,
                        0xD058, 0xD058, 0xD058, 0xD058};
    fake.regs[16] = 0x380U;
    fake.regs[8] = 0x100U;
    const auto add_loop = oasis::hybrid::generated::execute_0x000380(bridge, 0x380U);
    assert(add_loop.reason == oasis::hybrid::BlockExitReason::NORMAL_EXIT);
    assert(add_loop.instructions_executed == 16U && add_loop.next_pc == 0x3A0U);
    assert(fake.regs[8] == 0x120U);
    return 0;
}
