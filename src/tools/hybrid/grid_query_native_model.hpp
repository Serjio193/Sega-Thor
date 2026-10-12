#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <ostream>

namespace oasis::hybrid::grid_query {

using MemoryPeek = int (*)(unsigned address);

struct Observation {
    unsigned selector{};
    unsigned descriptor{};
    unsigned grid_base{};
    unsigned row_shift{};
    std::int16_t x{};
    std::int16_t y{};
    unsigned cell_address{};
    std::uint8_t cell_value{};
    unsigned d5{};
};

[[nodiscard]] std::optional<Observation> predict(
    const std::array<unsigned, 8>& data_registers,
    const std::array<unsigned, 8>& address_registers,
    MemoryPeek peek);

void write_json_fields(std::ostream& out,
                       const std::optional<Observation>& observation,
                       const std::optional<bool>& d5_match);

} // namespace oasis::hybrid::grid_query
