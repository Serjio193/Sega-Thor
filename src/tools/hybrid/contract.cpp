#include "tools/hybrid/contract.hpp"
#include "tools/re_static_translation.hpp"
#include <stdexcept>

namespace oasis::hybrid {
void require_mode(Mode mode) {
    if (mode == Mode::NATIVE_OVERRIDE) throw std::runtime_error(override_blocker);
}

Prediction predict(const State& entry, std::uint32_t return_pc,
                   std::span<const std::uint8_t> source, std::size_t output_bound) {
    if (entry[16] != target || source.size() < 4 || output_bound > 65536)
        throw std::runtime_error("0x3820 entry/footprint bound");
    Prediction result;
    result.output.resize(output_bound);
    std::vector<std::uint8_t> mechanical(output_bound);
    const auto native = game::decompress_graphics(source, result.output);
    const auto reference = tools::mechanical_3820(source, mechanical);
    mechanical.resize(reference.output_size);
    result.output.resize(native.output_size);
    if (reference.source_consumed != native.source_consumed || mechanical != result.output)
        throw std::runtime_error("FIRST_DIVERGENCE mechanical/native output or source consumption");
    result.consumed = native.source_consumed;
    result.state = entry;
    result.state[8] += static_cast<std::uint32_t>(native.source_consumed);
    result.state[9] += static_cast<std::uint32_t>(native.output_size);
    result.state[15] += 4;
    result.state[16] = return_pc;
    // Final MOVE.B of the zero terminator sets Z, clears N/V/C. MOVEM/RTS
    // preserve CCR. X is intentionally not predicted or claimed equivalent.
    result.state[17] = (entry[17] & 0xFFE0U) | 4;
    auto sp = entry[15];
    const auto push = [&](unsigned reg) {
        sp -= 4;
        // GPGX MOVEM predecrement writes the low word, then the high word.
        for (unsigned byte : {2U, 3U, 0U, 1U})
            result.stack_writes.push_back({sp + byte,
                static_cast<std::uint8_t>(entry[reg] >> (24 - 8 * byte))});
    };
    // 48E7 E020 at 3820, plus 48E7 1300 at 38D0 for format B.
    for (unsigned reg : {10U, 2U, 1U, 0U}) push(reg);
    if (source[2] == 0) for (unsigned reg : {7U, 6U, 3U}) push(reg);
    return result;
}

std::string compare_registers(const State& expected, const State& actual) {
    for (unsigned i = 0; i < expected.size(); ++i) {
        const auto mask = i == 17 ? proven_sr_mask : 0xFFFFFFFFU;
        if ((expected[i] & mask) != (actual[i] & mask))
            return "FIRST_DIVERGENCE register[" + std::to_string(i) + "] expected=" +
                std::to_string(expected[i] & mask) + " actual=" + std::to_string(actual[i] & mask);
    }
    return {};
}

std::string compare_bytes(std::span<const std::uint8_t> expected,
                          std::span<const std::uint8_t> actual, const char* region) {
    if (expected.size() != actual.size()) return std::string("FIRST_DIVERGENCE ") + region + " size";
    for (std::size_t i = 0; i < expected.size(); ++i)
        if (expected[i] != actual[i]) return std::string("FIRST_DIVERGENCE ") + region +
            " byte[" + std::to_string(i) + "]";
    return {};
}
}
