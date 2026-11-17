#include "game/entities/prng_lfsr.hpp"

#include <array>
#include <cassert>

namespace {

using namespace oasis::game::entities;

void test_first_8_outputs_from_7f28() {
    constexpr std::array<std::uint16_t, 8> kExpected = {
        0xF28AU, 0x28ABU, 0x8AB9U, 0xAB9EU,
        0xB9E6U, 0x9E65U, 0xE65DU, 0x65D7U,
    };

    PrngLfsr16 rng;
    assert(rng.state() == 0x7F28U);
    for (std::size_t i = 0; i < kExpected.size(); ++i) {
        assert(rng.next_rng16() == kExpected[i]);
    }
}

void test_bounded_random_helpers() {
    PrngLfsr16 rng;
    assert(rng.rand_u16(0U) == 0U);
    assert(rng.state() == 0x7F28U); // bound == 0 returns 0 without advancing

    assert(rng.rand_u8(0U) == 0U);
    assert(rng.state() == 0x7F28U);

    // (0xF28A * 100) >> 16 == 94
    assert(rng.rand_u16(100U) == static_cast<std::uint16_t>((0xF28AU * 100U) >> 16U));
    // (0x28AB * 10) >> 16 == 1
    assert(rng.rand_u8(10U) == static_cast<std::uint8_t>((0x28ABU * 10U) >> 16U));

    rng.reset();
    assert(rng.state() == 0x7F28U);
    assert(rng.next_rng16() == 0xF28AU);
}

} // namespace

int main() {
    test_first_8_outputs_from_7f28();
    test_bounded_random_helpers();
    return 0;
}
