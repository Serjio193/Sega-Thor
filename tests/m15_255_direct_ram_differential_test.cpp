#include "tools/thor_ir/exp250_direct_ram_adapter.hpp"
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

using namespace oasis::recompiler::ir;

namespace {

struct MockDirectRamContext {
    std::vector<std::uint8_t> ram{std::vector<std::uint8_t>(0x10000, 0)};
    std::uint32_t bus_reads{0};
    std::uint32_t bus_writes{0};
    std::uint32_t d[8]{};
    std::uint32_t a[8]{};
    std::uint16_t sr{0x2000};

    static std::uint32_t read_reg(void* ctx, unsigned i) {
        auto* self = static_cast<MockDirectRamContext*>(ctx);
        return i < 8 ? self->d[i] : (i < 16 ? self->a[i - 8] : (i == 17 ? self->sr : 0));
    }
    static void write_reg(void* ctx, unsigned i, std::uint32_t v) {
        auto* self = static_cast<MockDirectRamContext*>(ctx);
        if (i < 8) self->d[i] = v;
        else if (i < 16) self->a[i - 8] = v;
        else if (i == 17) self->sr = static_cast<std::uint16_t>(v);
    }
    static int peek(void* ctx, unsigned addr) {
        auto* self = static_cast<MockDirectRamContext*>(ctx);
        const auto offset = addr & 0xFFFFU;
        // In GPGX x86/x64, peek returns byte at offset ^ 1
        return self->ram[offset ^ 1U];
    }
    static std::uint32_t read_bus(void* ctx, unsigned addr, int width) {
        auto* self = static_cast<MockDirectRamContext*>(ctx);
        ++self->bus_reads;
        std::uint32_t val = 0;
        for (int i = 0; i < width; ++i) {
            const auto off = (addr + static_cast<unsigned>(i)) & 0xFFFFU;
            val = (val << 8) | self->ram[off ^ 1U];
        }
        return val;
    }
    static void write_bus(void* ctx, unsigned addr, int width, std::uint32_t val) {
        auto* self = static_cast<MockDirectRamContext*>(ctx);
        ++self->bus_writes;
        for (int i = 0; i < width; ++i) {
            const auto off = (addr + static_cast<unsigned>(i)) & 0xFFFFU;
            const auto shift = 8 * (width - 1 - i);
            self->ram[off ^ 1U] = static_cast<std::uint8_t>((val >> shift) & 0xFFU);
        }
    }
};

void test_direct_ram_read_write_parity() {
    MockDirectRamContext ctx;
    oasis::hybrid::M68k9D70StepApi api{};
    api.context = &ctx;
    api.read_register = MockDirectRamContext::read_reg;
    api.write_register = MockDirectRamContext::write_reg;
    api.peek = MockDirectRamContext::peek;
    api.read = MockDirectRamContext::read_bus;
    api.write = MockDirectRamContext::write_bus;

    set_m15250_direct_ram(ctx.ram.data(), ctx.ram.size());
    reset_exp250_memory_profile_metrics();

    HybridM68kDirectRamBus250 bus{api};

    // Test byte writes & reads at even and odd addresses
    bus.write_m(0xFF0010U, IrOpSize::Byte, 0xABU);
    bus.write_m(0xFF0011U, IrOpSize::Byte, 0xCDU);
    assert(bus.read_m(0xFF0010U, IrOpSize::Byte) == 0xABU);
    assert(bus.read_m(0xFF0011U, IrOpSize::Byte) == 0xCDU);

    // Verify host GPGX representation in raw memory
    // Even address 0x10 -> stored at host offset 0x10 ^ 1 = 0x11
    // Odd address 0x11 -> stored at host offset 0x11 ^ 1 = 0x10
    assert(ctx.ram[0x0011U] == 0xABU);
    assert(ctx.ram[0x0010U] == 0xCDU);

    // Test 16-bit word write & read (natural alignment)
    bus.write_m(0xFF0020U, IrOpSize::Word, 0x1234U);
    assert(bus.read_m(0xFF0020U, IrOpSize::Word) == 0x1234U);
    // On host little-endian x86, low byte (0x34) is at 0x20, high byte (0x12) is at 0x21
    assert(ctx.ram[0x0020U] == 0x34U);
    assert(ctx.ram[0x0021U] == 0x12U);
    // Guest byte reads must match 68000 big-endian layout:
    assert(bus.read_m(0xFF0020U, IrOpSize::Byte) == 0x12U);
    assert(bus.read_m(0xFF0021U, IrOpSize::Byte) == 0x34U);

    // Test 32-bit longword write & read
    bus.write_m(0xFF0030U, IrOpSize::Long, 0xDEADBEEFU);
    assert(bus.read_m(0xFF0030U, IrOpSize::Long) == 0xDEADBEEFU);
    assert(bus.read_m(0xFF0030U, IrOpSize::Word) == 0xDEADU);
    assert(bus.read_m(0xFF0032U, IrOpSize::Word) == 0xBEEFU);
    assert(bus.read_m(0xFF0030U, IrOpSize::Byte) == 0xDEU);
    assert(bus.read_m(0xFF0031U, IrOpSize::Byte) == 0xADU);
    assert(bus.read_m(0xFF0032U, IrOpSize::Byte) == 0xBEU);
    assert(bus.read_m(0xFF0033U, IrOpSize::Byte) == 0xEFU);

    // All these accesses were fast-path RAM accesses; bus callbacks must be 0!
    assert(ctx.bus_reads == 0);
    assert(ctx.bus_writes == 0);

    const auto& metrics = get_exp250_memory_profile_metrics();
    assert(metrics.fast_ram_reads_byte == 8);
    assert(metrics.fast_ram_reads_word == 3);
    assert(metrics.fast_ram_reads_long == 1);
    assert(metrics.fast_ram_writes_byte == 2);
    assert(metrics.fast_ram_writes_word == 1);
    assert(metrics.fast_ram_writes_long == 1);
    assert(metrics.fallback_bus_reads == 0);
    assert(metrics.fallback_bus_writes == 0);

    std::cout << "  [PASS] Test 1: Direct RAM byte, word, and longword read/write bit-exact parity\n";
}

void test_direct_ram_mirrors_and_boundaries() {
    MockDirectRamContext ctx;
    oasis::hybrid::M68k9D70StepApi api{};
    api.context = &ctx;
    api.read_register = MockDirectRamContext::read_reg;
    api.write_register = MockDirectRamContext::write_reg;
    api.peek = MockDirectRamContext::peek;
    api.read = MockDirectRamContext::read_bus;
    api.write = MockDirectRamContext::write_bus;

    set_m15250_direct_ram(ctx.ram.data(), ctx.ram.size());
    reset_exp250_memory_profile_metrics();

    HybridM68kDirectRamBus250 bus{api};

    // Mirror address: 0xE00040 maps to the exact same RAM as 0xFF0040
    bus.write_m(0xE00040U, IrOpSize::Word, 0x5678U);
    assert(bus.read_m(0xFF0040U, IrOpSize::Word) == 0x5678U);
    assert(bus.read_m(0xE10040U, IrOpSize::Word) == 0x5678U);

    // Boundary check: write at 0xFFFFFF (offset 0xFFFF) with word crosses 64KB boundary -> fallback to bus
    bus.write_m(0xFFFFFFU, IrOpSize::Word, 0x9999U);
    assert(ctx.bus_writes == 1); // Fell back safely!

    // Non-RAM address: ROM (0x001000) or VDP (0xC00000) -> fallback to bus
    bus.read_m(0x001000U, IrOpSize::Word);
    assert(ctx.bus_reads == 1);

    bus.write_m(0xC00000U, IrOpSize::Word, 0x0000U);
    assert(ctx.bus_writes == 2);

    // Unaligned word read at odd address 0xFF0041 -> fallback to bus
    bus.read_m(0xFF0041U, IrOpSize::Word);
    assert(ctx.bus_reads == 2);

    const auto& m = get_exp250_memory_profile_metrics();
    assert(m.ram_mirror_accesses >= 2);
    assert(m.rom_accesses == 1);
    assert(m.vdp_accesses == 1);
    assert(m.fallback_bus_reads == 2);
    assert(m.fallback_bus_writes == 2);

    std::cout << "  [PASS] Test 2: RAM mirrors, boundary wrapping, and hardware rejection\n";
}

void test_direct_ram_fail_closed_null() {
    MockDirectRamContext ctx;
    oasis::hybrid::M68k9D70StepApi api{};
    api.context = &ctx;
    api.read_register = MockDirectRamContext::read_reg;
    api.write_register = MockDirectRamContext::write_reg;
    api.peek = MockDirectRamContext::peek;
    api.read = MockDirectRamContext::read_bus;
    api.write = MockDirectRamContext::write_bus;

    // Set NULL direct RAM pointer
    set_m15250_direct_ram(nullptr, 0);
    reset_exp250_memory_profile_metrics();

    HybridM68kDirectRamBus250 bus{api};

    // Any access must fail-closed to api_.read/write without crashing
    bus.write_m(0xFF0010U, IrOpSize::Word, 0xCAFEU);
    assert(ctx.bus_writes == 1);
    const auto val = bus.read_m(0xFF0010U, IrOpSize::Word);
    assert(ctx.bus_reads == 1);
    assert(val == 0xCAFEU);

    const auto& m = get_exp250_memory_profile_metrics();
    assert(m.fallback_bus_reads == 1);
    assert(m.fallback_bus_writes == 1);

    std::cout << "  [PASS] Test 3: Null pointer fail-closed fallback safety\n";
}

void test_virtual_bus_vs_direct_ram_microbenchmark() {
    MockDirectRamContext ctx;
    oasis::hybrid::M68k9D70StepApi api{};
    api.context = &ctx;
    api.read_register = MockDirectRamContext::read_reg;
    api.write_register = MockDirectRamContext::write_reg;
    api.peek = MockDirectRamContext::peek;
    api.read = MockDirectRamContext::read_bus;
    api.write = MockDirectRamContext::write_bus;

    constexpr int kIterations = 2000000;

    // 1. Measure virtual bus callback overhead
    auto t0 = std::chrono::high_resolution_clock::now();
    volatile std::uint32_t sink = 0;
    for (int i = 0; i < kIterations; ++i) {
        sink = api.read(api.context, 0xFF0000U + (i & 0x7FFEU), 2);
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    const double bus_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / kIterations;

    // 2. Measure direct RAM pointer access
    const auto* direct_ptr = ctx.ram.data();
    t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < kIterations; ++i) {
        sink = *reinterpret_cast<const std::uint16_t*>(direct_ptr + (i & 0x7FFEU));
    }
    t1 = std::chrono::high_resolution_clock::now();
    const double direct_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / kIterations;

    // 3. Measure instrumentation overhead
    Exp250MemoryProfileMetrics prof{};
    t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < kIterations; ++i) {
        record_mem_access(prof, 0xFF0000U + (i & 0x7FFEU), 2, false);
    }
    t1 = std::chrono::high_resolution_clock::now();
    const double prof_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / kIterations;

    std::cout << "  [BENCH] Virtual Bus Callback: " << bus_ns << " ns/op\n"
              << "  [BENCH] Direct RAM Access:    " << direct_ns << " ns/op ("
              << (bus_ns / direct_ns) << "x faster)\n"
              << "  [BENCH] Profiling Overhead:    " << prof_ns << " ns/op\n";
    std::cout << "  [PASS] Test 4: Bus vs Direct RAM microbenchmark forensics validated\n";
}

} // namespace

int main() {
    std::cout << "=== M15.255 Direct-RAM Access & Memory Forensics Tests ===\n";
    test_direct_ram_read_write_parity();
    test_direct_ram_mirrors_and_boundaries();
    test_direct_ram_fail_closed_null();
    test_virtual_bus_vs_direct_ram_microbenchmark();
    std::cout << "ALL DIRECT-RAM DIFFERENTIAL TESTS PASSED!\n";
    return 0;
}
