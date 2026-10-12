#include "tools/hybrid/gameplay_session_cpu.hpp"

#include <sstream>
#include <stdexcept>

namespace oasis::hybrid::session {

CpuSnapshot read_cpu_snapshot(RegisterRead read_register) {
    CpuSnapshot cpu;
    for (unsigned i = 0; i < 8; ++i) {
        cpu.d[i] = read_register(i);
        cpu.a[i] = read_register(i + 8U);
    }
    cpu.pc = read_register(16U) & 0xFFFFFFU;
    cpu.sr = read_register(17U) & 0xFFFFU;
    return cpu;
}

std::string serialize_cpu_snapshot(const CpuSnapshot& cpu) {
    std::ostringstream out;
    out << "{\"pc\":" << cpu.pc << ",\"sr\":" << cpu.sr << ",\"d\":[";
    for (unsigned i = 0; i < 8; ++i) out << (i ? "," : "") << cpu.d[i];
    out << "],\"a\":[";
    for (unsigned i = 0; i < 8; ++i) out << (i ? "," : "") << cpu.a[i];
    out << "]}";
    return out.str();
}

unsigned read_be(Peek peek_memory, unsigned address, unsigned width) {
    unsigned value{};
    for (unsigned i = 0; i < width; ++i) {
        const auto byte = peek_memory((address + i) & 0xFFFFFFU);
        if (byte < 0) throw std::runtime_error("CPU bridge could not read memory");
        value = (value << 8U) | static_cast<unsigned>(byte & 0xFF);
    }
    return value;
}

std::vector<std::uint8_t> read_work_ram(Peek peek_memory) {
    std::vector<std::uint8_t> bytes(0x10000U);
    for (unsigned i = 0; i < bytes.size(); ++i) {
        const auto value = peek_memory(0xFF0000U + i);
        if (value < 0) throw std::runtime_error("CPU bridge could not snapshot work RAM");
        bytes[i] = static_cast<std::uint8_t>(value);
    }
    return bytes;
}

std::vector<std::uint8_t> serialize_core_state(
    std::size_t (*size_fn)(), bool (*serialize_fn)(void*, std::size_t)) {
    const auto size = size_fn();
    if (!size || size > 64U * 1024U * 1024U) throw std::runtime_error("invalid core state size");
    std::vector<std::uint8_t> state(size);
    if (!serialize_fn(state.data(), state.size())) throw std::runtime_error("core serialization failed");
    return state;
}

} // namespace oasis::hybrid::session
