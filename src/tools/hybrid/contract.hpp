#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace oasis::hybrid {
enum class Mode { EMULATED, SHADOW_NATIVE, NATIVE_OVERRIDE };
using State = std::array<std::uint32_t, 18>; // D0-D7, A0-A7, PC, SR; developer-only.
struct Write { std::uint32_t address; std::uint8_t value; };
struct Prediction {
    State state{};
    std::vector<std::uint8_t> output;
    std::vector<Write> stack_writes;
    std::size_t consumed{};
};
inline constexpr std::uint32_t target = 0x3820;
// The existing decoders have no instruction-level X or timing model.
inline constexpr std::uint16_t proven_sr_mask = 0xFFEF;
inline constexpr const char* override_blocker =
    "CCR.X and instruction/cycle/bus-refresh/interrupt scheduling are not modeled; "
    "prefetch/IR continuation has no validated adapter contract";
void require_mode(Mode mode);
Prediction predict(const State& entry, std::uint32_t return_pc,
                   std::span<const std::uint8_t> source, std::size_t output_bound);
std::string compare_registers(const State& expected, const State& actual);
std::string compare_bytes(std::span<const std::uint8_t> expected,
                          std::span<const std::uint8_t> actual, const char* region);
}
