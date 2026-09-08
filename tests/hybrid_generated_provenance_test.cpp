#include "tools/hybrid/generated_blocks.hpp"

#include <cassert>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace {

struct Fake {
    std::uint32_t regs[18]{};
    std::unordered_map<std::uint32_t, std::uint8_t> memory;
    std::vector<unsigned> fetch_words;
    unsigned fetch_index{};
    static Fake* current;

    static unsigned reg(unsigned index) { return current->regs[index]; }
    static void set_reg(unsigned index, unsigned value) { current->regs[index] = value; }
    static int peek(unsigned address) {
        const auto it = current->memory.find(address);
        return it == current->memory.end() ? 0U : it->second;
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
    static unsigned fetch16() {
        const auto value = current->fetch_words.at(current->fetch_index++);
        current->regs[16] += 2U;
        return value;
    }
    static void begin(unsigned) {}
    static void finish(unsigned) {}
    static void add_cycles(int) {}
    static void skip_refresh() {}
};

Fake* Fake::current = nullptr;

oasis::hybrid::BasicBlockApi api() {
    return {Fake::reg, Fake::set_reg, Fake::peek, Fake::fetch16, Fake::read, Fake::write,
            Fake::begin, Fake::finish, Fake::add_cycles, Fake::skip_refresh, nullptr,
            nullptr, nullptr, nullptr};
}

void run_block(std::uint32_t start, std::uint32_t end,
               std::vector<unsigned> words,
               oasis::hybrid::GeneratedBlockExecutor execute) {
    Fake fake;
    Fake::current = &fake;
    fake.regs[7] = 0x11223344U;
    fake.regs[11] = 0x00FF2000U;
    fake.regs[14] = 0x100U;
    fake.regs[15] = 0xFFFF00U;
    fake.regs[16] = start;
    fake.regs[17] = 0x2713U;
    fake.memory[0x100U] = 0x02U;
    fake.memory[0x101U] = 0x01U;
    fake.memory[0x102U] = 0x12U;
    fake.memory[0x103U] = 0x34U;
    fake.memory[0xFF1654U] = 0x12U;
    fake.memory[0xFF1655U] = 0x34U;
    fake.memory[0xFF0BFDU] = 0x80U;
    fake.fetch_words = std::move(words);
    auto bridge = api();
    const auto exit = execute(bridge, start);
    assert(exit.reason == oasis::hybrid::BlockExitReason::NORMAL_EXIT);
    assert(exit.instructions_executed > 0U);
    assert(fake.regs[16] == end);
    assert(fake.fetch_index == fake.fetch_words.size());
    Fake::current = nullptr;
}

} // namespace

int main() {
    run_block(0x2D66U, 0x2D7AU,
              {0x48E7, 0x0110, 0x4247, 0x1E1E, 0x47F9, 0x00FF, 0x134C,
               0xD6C7, 0x1E1E, 0x36DE},
              oasis::hybrid::generated::execute_0x002D66);
    run_block(0x604BCU, 0x604C2U, {0x4DF9, 0x00FF, 0x0628},
              oasis::hybrid::generated::execute_0x0604BC);
    run_block(0x61032U, 0x61034U, {0xD481},
              oasis::hybrid::generated::execute_0x061032);
    run_block(0x3A85EU, 0x3A864U, {0x4A79, 0x00FF, 0x1654},
              oasis::hybrid::generated::execute_0x03A85E);
    run_block(0x3A8BAU, 0x3A8C0U, {0x4A79, 0x00FF, 0x1654},
              oasis::hybrid::generated::execute_0x03A8BA);
    run_block(0x3A88CU, 0x3A892U, {0x4A39, 0x00FF, 0x0BFD},
              oasis::hybrid::generated::execute_0x03A88C);
    return 0;
}
