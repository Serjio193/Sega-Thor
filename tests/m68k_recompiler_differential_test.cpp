#include "tools/hybrid/generated/m68k_recompiled_002512.hpp"
#include "tools/hybrid/m68k_002512_instruction_step.hpp"
#include "tools/hybrid/m68k_002512_step_ops.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed at " << __FILE__ << ":" << __LINE__ << ": " #cond << "\n"; \
            std::exit(1); \
        } \
    } while (false)

namespace {

using namespace oasis::hybrid;
using namespace oasis::hybrid::generated;

struct MockGuestState {
    std::array<std::uint32_t, 18> regs{};
    std::map<std::uint32_t, std::uint8_t> mem{};
    std::vector<std::pair<unsigned, std::uint16_t>> vdp_writes{};
    int cycles{0};
    unsigned last_pc{0};

    std::uint32_t read_reg(unsigned r) const { return r < regs.size() ? regs[r] : 0U; }
    void write_reg(unsigned r, std::uint32_t v) { if (r < regs.size()) regs[r] = v; }
    std::uint32_t read_mem(unsigned addr, int width) const {
        addr &= 0xFFFFFFU;
        std::uint32_t val = 0U;
        for (int i = 0; i < width; ++i) {
            auto it = mem.find(addr + i);
            val = (val << 8U) | (it != mem.end() ? it->second : 0U);
        }
        return val;
    }
    void write_mem(unsigned addr, int width, std::uint32_t val) {
        addr &= 0xFFFFFFU;
        if (addr == 0xC00000U || addr == 0xC00004U) {
            vdp_writes.push_back({addr, static_cast<std::uint16_t>(val & 0xFFFFU)});
        }
        for (int i = width - 1; i >= 0; --i) {
            mem[addr + i] = static_cast<std::uint8_t>(val & 0xFFU);
            val >>= 8U;
        }
    }
};

MockGuestState g_state_rec;
MockGuestState g_state_hnd;

std::uint32_t mock_read_reg(void* ctx, unsigned r) { return static_cast<MockGuestState*>(ctx)->read_reg(r); }
void mock_write_reg(void* ctx, unsigned r, std::uint32_t v) { static_cast<MockGuestState*>(ctx)->write_reg(r, v); }
int mock_peek(void* ctx, unsigned a) { return static_cast<int>(static_cast<MockGuestState*>(ctx)->read_mem(a, 1)); }
std::uint32_t mock_read(void* ctx, unsigned a, int w) { return static_cast<MockGuestState*>(ctx)->read_mem(a, w); }
void mock_write(void* ctx, unsigned a, int w, std::uint32_t v) { static_cast<MockGuestState*>(ctx)->write_mem(a, w, v); }
std::uint32_t mock_fetch16(void*) { return 0x4E71U; }
void mock_set_return_state(void* ctx, unsigned pc, unsigned, unsigned) { static_cast<MockGuestState*>(ctx)->last_pc = pc; }
void mock_begin_inst(void*, unsigned) {}
void mock_finish_inst(void*, unsigned) {}
void mock_add_cycles(void* ctx, int c) { static_cast<MockGuestState*>(ctx)->cycles += c; }
int mock_word_not_taken(void*) { return 14; }
int mock_byte_not_taken(void*) { return 12; }
void mock_observe(void*, unsigned, unsigned) {}
void mock_skip_bus(void*) {}

M68k9D70StepApi make_api(MockGuestState& state) {
    M68k9D70StepApi api{};
    api.context = &state;
    api.read_register = mock_read_reg;
    api.write_register = mock_write_reg;
    api.peek = mock_peek;
    api.read = mock_read;
    api.write = mock_write;
    api.fetch16 = mock_fetch16;
    api.set_return_state = mock_set_return_state;
    api.begin_instruction = mock_begin_inst;
    api.finish_instruction = mock_finish_inst;
    api.add_cycles = mock_add_cycles;
    api.word_not_taken_adjustment = mock_word_not_taken;
    api.byte_not_taken_adjustment = mock_byte_not_taken;
    api.observe_instruction = mock_observe;
    api.skip_bus_refresh = mock_skip_bus;
    return api;
}

void init_guest_scenario(MockGuestState& state, bool scroll_down) {
    state.regs.fill(0);
    state.mem.clear();
    state.vdp_writes.clear();
    state.cycles = 0;
    state.last_pc = 0;

    const unsigned a3 = 0xFF1716U;
    state.regs[step_002512::kRegA3] = a3;
    const unsigned tile_data_addr = 0xFF5000U;
    
    // RAM context parameters
    state.write_mem(a3 + 22U, 4, tile_data_addr);
    state.write_mem(a3 + 26U, 2, 0xC000U); // Base VRAM dest (Plane A)
    state.write_mem(a3 + 30U, 2, scroll_down ? 0xFFFFU : 0x0001U);
    state.write_mem(a3 + 12U, 2, 0x0080U);
    state.write_mem(a3 + 32U, 2, 0x0080U);
    state.write_mem(a3 + 34U, 2, 0x0020U);
    state.write_mem(a3 + 8U,  2, 0x0004U);
    state.write_mem(a3 + 10U, 2, 0x0002U);
    state.write_mem(a3 + 36U, 2, 0x0007U);

    for (unsigned i = 0; i < 64; ++i) {
        state.write_mem(tile_data_addr + i * 2U, 2, 0x0180U + i);
    }

    const unsigned sp = 0xFFFF40U;
    state.regs[step_002512::kRegSP] = sp;
    state.write_mem(sp, 4, 0x0020C8U); // Return address
}

void setup_rom_words(MockGuestState& state) {
    static const std::uint16_t kRomSlice[] = {
        0x206B, 0x0016, 0x3A2B, 0x001A, 0x45F9, 0x00C0, 0x0000, 0x49EA, 0x0004,
        0x3C2B, 0x001E, 0x6B00, 0x006C, 0x322B, 0x000C, 0x5346, 0x3746, 0x001E,
        0x362B, 0x0020, 0x382B, 0x0022, 0x342B, 0x0008, 0x3002, 0xD040, 0x3E2B,
        0x000A, 0x5247, 0xBE44, 0x6D00, 0x0004, 0x9E44, 0x3747, 0x000A, 0x0647,
        0x001C, 0xBE44, 0x6D00, 0x0004, 0x9E44, 0x3C2B, 0x0024, 0xED6F, 0xDE40,
        0xD0C7, 0x3801, 0x0241, 0x0F80, 0x0641, 0x0080, 0x0244, 0x007E, 0x0241,
        0x0F80, 0x3E01, 0xDE44, 0x3747, 0x000C, 0x0641, 0x0E00, 0x0241, 0x0F80,
        0xD244, 0x6000, 0x004E, 0x322B, 0x000C, 0x5246, 0x3746, 0x001E, 0x362B,
        0x0020, 0x382B, 0x0022, 0x342B, 0x0008, 0x3002, 0xD040, 0x3E2B, 0x000A,
        0x5347, 0x6A00, 0x0004, 0xDE44, 0x3747, 0x000A, 0x3C2B, 0x0024, 0xED6F,
        0xDE40, 0xD0C7, 0x3801, 0x0241, 0x0F80, 0x0441, 0x0080, 0x0244, 0x007E,
        0x0241, 0x0F80, 0xD244, 0x3741, 0x000C, 0xE24C, 0x3C05, 0xE55D, 0x0245,
        0x0003, 0x0246, 0x3FFF, 0x0046, 0x4000, 0x3006, 0xD041, 0x3880, 0x3885,
        0x3E3C, 0x0028, 0x3498, 0x5242, 0xB443, 0x6D00, 0x0008, 0x9443, 0x90C3,
        0x90C3, 0x0641, 0x0002, 0x5244, 0x0C44, 0x0040, 0x6D00, 0x0010, 0x0444,
        0x0040, 0x0441, 0x0080, 0xDC41, 0x3886, 0x3885, 0x51CF, 0xFFD2, 0x4E75
    };
    unsigned pc = 0x002512U;
    for (auto w : kRomSlice) {
        state.write_mem(pc, 2, w);
        pc += 2U;
    }
}

void test_differential_case(bool scroll_down) {
    init_guest_scenario(g_state_rec, scroll_down);
    init_guest_scenario(g_state_hnd, scroll_down);
    setup_rom_words(g_state_rec);
    setup_rom_words(g_state_hnd);

    auto api_rec = make_api(g_state_rec);
    auto api_hnd = make_api(g_state_hnd);

    // 1. Run recompiled generator prototype
    unsigned resume_pc_rec = 0;
    auto res_rec = m68k_recompiled_002512(api_rec, resume_pc_rec);
    TEST_ASSERT(res_rec == M68k9D70StepResult::InstructionStepped);
    TEST_ASSERT(resume_pc_rec == 0x0020C8U);

    // 2. Run qualified handwritten stepper step by step
    unsigned pc_hnd = 0x002512U;
    unsigned steps = 0;
    oasis::hybrid::reset_002512_state();
    while (pc_hnd != 0x0020C8U && steps < 500) {
        auto res_hnd = step_m68k_002512(pc_hnd, api_hnd, true);
        TEST_ASSERT(res_hnd == M68k9D70StepResult::InstructionStepped);
        pc_hnd = g_state_hnd.last_pc;
        steps++;
    }

    // 3. Differential assertions: registers D0-D7, A0-A7
    for (unsigned r = 0; r < 16; ++r) {
        TEST_ASSERT(g_state_rec.read_reg(r) == g_state_hnd.read_reg(r));
    }

    // 4. Differential assertions: VDP write port count and sequence
    TEST_ASSERT(g_state_rec.vdp_writes.size() == g_state_hnd.vdp_writes.size());
    for (size_t i = 0; i < g_state_rec.vdp_writes.size(); ++i) {
        TEST_ASSERT(g_state_rec.vdp_writes[i] == g_state_hnd.vdp_writes[i]);
    }

    // 5. Differential assertions: RAM context
    for (unsigned off = 0; off < 40; ++off) {
        const unsigned addr = 0xFF1716U + off;
        TEST_ASSERT(g_state_rec.read_mem(addr, 1) == g_state_hnd.read_mem(addr, 1));
    }

    std::cout << "[PASS] Differential Test (scroll_down=" << scroll_down 
              << "): Recompiled C++ matches Handwritten Stepper bit-exact! (writes=" 
              << g_state_rec.vdp_writes.size() << ")\n";
}

} // namespace

int main() {
    test_differential_case(false); // Scroll up/right
    test_differential_case(true);  // Scroll down/left
    std::cout << "ALL RECOMPILER DIFFERENTIAL TESTS PASSED WITH 100% BIT-EXACT PARITY!\n";
    return 0;
}
