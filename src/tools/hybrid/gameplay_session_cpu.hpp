#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace oasis::hybrid::session {

struct CpuSnapshot {
    std::array<unsigned, 8> d{};
    std::array<unsigned, 8> a{};
    unsigned pc{};
    unsigned sr{};
};

using RegisterRead = unsigned (*)(unsigned);
using Peek = int (*)(unsigned);

[[nodiscard]] CpuSnapshot read_cpu_snapshot(RegisterRead read_register);
[[nodiscard]] std::string serialize_cpu_snapshot(const CpuSnapshot& cpu);
[[nodiscard]] unsigned read_be(Peek peek_memory, unsigned address, unsigned width);
[[nodiscard]] std::vector<std::uint8_t> read_work_ram(Peek peek_memory);
[[nodiscard]] std::vector<std::uint8_t> serialize_core_state(
    std::size_t (*size_fn)(), bool (*serialize_fn)(void*, std::size_t));

} // namespace oasis::hybrid::session
