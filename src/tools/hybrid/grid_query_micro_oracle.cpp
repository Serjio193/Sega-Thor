#include "tools/hybrid/grid_query_micro_oracle.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace oasis::hybrid {
namespace {
constexpr unsigned execute = 1;
constexpr unsigned memory_read = 2;
constexpr unsigned entry = 0xFF6000;
constexpr unsigned sentinel = 0xFF6100;
constexpr unsigned stack = 0xFF20F0;
const MicroBridge* active_bridge{};
MicroOracleResult* active_result{};
bool sentinel_seen{};

std::string hex(unsigned value, unsigned digits = 8) {
    std::ostringstream out;
    out << "0x" << std::hex << std::setfill('0') << std::setw(static_cast<int>(digits)) << value;
    return out.str();
}

unsigned peek_word(unsigned address) {
    const int hi = active_bridge->peek_memory(address);
    const int lo = active_bridge->peek_memory(address + 1);
    if (hi < 0 || lo < 0) return 0xFFFFFFFFU;
    return (static_cast<unsigned>(hi & 0xFF) << 8) | static_cast<unsigned>(lo & 0xFF);
}

MicroCpuState snapshot() {
    MicroCpuState state;
    for (unsigned i = 0; i < 8; ++i) {
        state.data[i] = active_bridge->read_register(i);
        state.address[i] = active_bridge->read_register(8 + i);
    }
    state.pc = active_bridge->read_register(16);
    state.sr = active_bridge->read_register(17) & 0xFFFFU;
    return state;
}

unsigned int hook(int type, int width, unsigned address, unsigned value) {
    if ((type & execute) != 0) {
        active_result->steps.push_back({address, active_bridge->read_register(16),
            peek_word(address), active_bridge->read_register(0),
            active_bridge->read_register(1), active_bridge->read_register(14),
            active_bridge->read_register(15), active_bridge->read_register(17) & 0xFFFFU});
        if (address == sentinel) {
            sentinel_seen = true;
            active_result->sentinel_state = snapshot();
            active_bridge->stop_cpu();
        }
    }
    if ((type & memory_read) != 0 && width == 4 && address == stack)
        active_result->rts_cpu_read_value = value;
    // HOOK_CPU uses the callback return as the value delivered to the CPU.
    return value;
}

} // namespace

MicroOracleResult run_m68k_rts_micro_oracle(const MicroBridge& bridge) {
    MicroOracleResult result;
    active_bridge = &bridge;
    active_result = &result;
    sentinel_seen = false;
    auto write = [&](unsigned address, int width, unsigned value) {
        if (!bridge.write_memory(address, width, value))
            throw std::runtime_error("micro-oracle work-RAM write rejected");
    };

    write(entry, 2, 0x702A);       // MOVEQ #42,D0
    write(entry + 2, 2, 0x220E);   // MOVE.L A6,D1
    write(entry + 4, 2, 0x4E75);   // RTS
    write(sentinel, 2, 0x4E71);    // NOP sentinel
    write(stack, 4, 0x00FF6100);
    for (unsigned i = 0; i < 4; ++i) {
        const int byte = bridge.peek_memory(stack + i);
        if (byte < 0) throw std::runtime_error("micro-oracle stack byte unavailable");
        result.raw_stack[i] = static_cast<unsigned>(byte & 0xFF);
    }
    result.bridge_stack_longword = (result.raw_stack[0] << 24) |
        (result.raw_stack[1] << 16) | (result.raw_stack[2] << 8) | result.raw_stack[3];

    bridge.write_register(17, 0x2000);
    bridge.write_register(0, 0x12345678);
    bridge.write_register(1, 0xA1B2C3D4);
    bridge.write_register(14, 0xFF234560);
    bridge.write_register(15, stack);
    bridge.write_register(16, entry);
    bridge.install_hook(hook);
    bridge.run_cpu(10000);
    bridge.install_hook(nullptr);

    const auto& s = result.steps;
    const bool sequence_ok = s.size() == 4 &&
        s[0].hook_address == entry && s[0].register_pc == entry &&
        s[0].opcode == 0x702A && s[0].d0 == 0x12345678 &&
        s[0].d1 == 0xA1B2C3D4 && s[0].a6 == 0xFF234560 &&
        s[0].a7 == stack && s[0].sr == 0x2000 &&
        s[1].hook_address == entry + 2 && s[1].register_pc == entry + 2 &&
        s[1].opcode == 0x220E && s[1].d0 == 42 && s[1].d1 == 0xA1B2C3D4 &&
        s[2].hook_address == entry + 4 && s[2].register_pc == entry + 4 &&
        s[2].opcode == 0x4E75 && s[2].d0 == 42 && s[2].d1 == 0xFF234560 &&
        s[3].hook_address == sentinel && s[3].register_pc == sentinel &&
        s[3].opcode == 0x4E71 && s[3].d0 == 42 && s[3].d1 == 0xFF234560;
    const auto& post = result.sentinel_state;
    result.passed = sentinel_seen && result.bridge_stack_longword == 0x00FF6100 &&
        result.rts_cpu_read_value == 0x00FF6100 && sequence_ok &&
        post.pc == sentinel && post.data[0] == 42 && post.data[1] == 0xFF234560 &&
        post.address[6] == 0xFF234560 && post.address[7] == stack + 4 && post.sr == 0x2008;
    if (!result.passed) result.failure = "instruction/stack/register/sentinel state mismatch";
    active_result = nullptr;
    active_bridge = nullptr;
    return result;
}

void write_micro_oracle_json(std::ostream& out, const MicroOracleResult& result) {
    out << "{\"status\":\"" << (result.passed ? "PASS" : "FAIL")
        << "\",\"failure\":\"" << result.failure
        << "\",\"hook_phase\":\"EXEC callback before opcode fetch/decode\","
        << "\"memory_read_phase\":\"bus value supplied after read; callback return consumed by CPU\","
        << "\"stack_raw_bytes\":[";
    for (unsigned i = 0; i < 4; ++i) out << (i ? "," : "") << result.raw_stack[i];
    out << "],\"bridge_stack_longword\":\"" << hex(result.bridge_stack_longword)
        << "\",\"rts_cpu_read_value\":\"" << hex(result.rts_cpu_read_value)
        << "\",\"sentinel_state\":{\"PC\":\"" << hex(result.sentinel_state.pc, 6)
        << "\",\"SR\":\"" << hex(result.sentinel_state.sr, 4)
        << "\",\"D0\":\"" << hex(result.sentinel_state.data[0])
        << "\",\"D1\":\"" << hex(result.sentinel_state.data[1])
        << "\",\"A6\":\"" << hex(result.sentinel_state.address[6], 6)
        << "\",\"A7\":\"" << hex(result.sentinel_state.address[7], 6) << "\"},\"steps\":[";
    for (std::size_t i = 0; i < result.steps.size(); ++i) {
        const auto& step = result.steps[i];
        out << (i ? "," : "") << "{\"hook_address\":\"" << hex(step.hook_address, 6)
            << "\",\"register_pc\":\"" << hex(step.register_pc, 6)
            << "\",\"opcode\":\"" << hex(step.opcode, 4)
            << "\",\"D0\":\"" << hex(step.d0) << "\",\"D1\":\"" << hex(step.d1)
            << "\",\"A6\":\"" << hex(step.a6, 6) << "\",\"A7\":\""
            << hex(step.a7, 6) << "\",\"SR\":\"" << hex(step.sr, 4) << "\"}";
    }
    out << "]}";
}

} // namespace oasis::hybrid
