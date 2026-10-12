#include "tools/hybrid/grid_query_native_model.hpp"

#include "game/world/byte_grid.hpp"

#include <ostream>
#include <vector>

namespace oasis::hybrid::grid_query {
namespace {
std::optional<unsigned> read_be(MemoryPeek peek, unsigned address, unsigned width) noexcept {
    unsigned value{};
    for (unsigned i = 0; i < width; ++i) {
        const int byte = peek((address + i) & 0xFFFFFFU);
        if (byte < 0) return std::nullopt;
        value = (value << 8) | static_cast<unsigned>(byte & 0xFF);
    }
    return value;
}
} // namespace

std::optional<Observation> predict(const std::array<unsigned, 8>& d,
                                  const std::array<unsigned, 8>& a,
                                  MemoryPeek peek) {
    if (!peek) return std::nullopt;
    const auto selector = read_be(peek, 0xFF16F0U, 1);
    if (!selector) return std::nullopt;
    Observation out;
    out.selector = *selector;
    out.descriptor = (*selector & 0x80U) ? 0xFF1770U : 0xFF1766U;
    const auto base = read_be(peek, out.descriptor, 4);
    const auto shift = read_be(peek, out.descriptor + 8U, 2);
    const auto x = read_be(peek, (a[6] + 8U) & 0xFFFFFFU, 2);
    const auto y = read_be(peek, (a[6] + 12U) & 0xFFFFFFU, 2);
    if (!base || !shift || !x || !y || *shift != 6U) return std::nullopt;
    out.grid_base = *base & 0xFFFFFFU;
    out.row_shift = *shift;
    out.x = static_cast<std::int16_t>(*x);
    out.y = static_cast<std::int16_t>(*y);
    if (out.grid_base > 0xFFFFFFU - 4095U || out.x < 0 || out.y < 0) return std::nullopt;

    std::vector<std::uint8_t> bytes(4096U);
    for (unsigned i = 0; i < bytes.size(); ++i) {
        const int value = peek(out.grid_base + i);
        if (value < 0) return std::nullopt;
        bytes[i] = static_cast<std::uint8_t>(value);
    }
    const game::world::ByteGridView view(bytes, 64, 64, 64, 6);
    const auto cell = view.sample_world(out.x, out.y);
    if (!cell) return std::nullopt;
    out.cell_value = *cell;
    out.cell_address = out.grid_base + static_cast<unsigned>(out.x / 8) +
        (static_cast<unsigned>(out.y / 8) << out.row_shift);
    out.d5 = (d[5] & 0xFFFFFF00U) | *cell;
    return out;
}

void write_json_fields(std::ostream& out,
                       const std::optional<Observation>& observation,
                       const std::optional<bool>& d5_match) {
    out << "\"native_grid\":";
    if (!observation) out << "null";
    else {
        const auto& n = *observation;
        out << "{\"selector\":" << n.selector << ",\"descriptor\":" << n.descriptor
            << ",\"grid_base\":" << n.grid_base << ",\"row_shift\":" << n.row_shift
            << ",\"x\":" << n.x << ",\"y\":" << n.y << ",\"cell_address\":"
            << n.cell_address << ",\"cell_value\":" << static_cast<unsigned>(n.cell_value)
            << ",\"d5\":" << n.d5 << '}';
    }
    out << ",\"native_d5_match\":";
    if (d5_match) out << (*d5_match ? "true" : "false");
    else out << "null";
}

} // namespace oasis::hybrid::grid_query
