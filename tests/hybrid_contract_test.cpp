#include "tools/hybrid/contract.hpp"
#include <iostream>
#include <stdexcept>

void check(bool value) { if (!value) throw std::runtime_error("hybrid contract regression"); }
int main() {
    using namespace oasis::hybrid;
    try {
        State entry{};
        for (unsigned i = 0; i < 16; ++i) entry[i] = 0x12340000 + i;
        entry[8] = 0x1000; entry[9] = 0xFF1000; entry[15] = 0xFFFF00;
        entry[16] = target; entry[17] = 0x271F;
        // TEST FIXTURE: format A, 3 literal bytes, zero terminator.
        const std::array<std::uint8_t, 7> source{6, 0, 3, 0xA5, 0x5A, 0x11, 0};
        const auto result = predict(entry, 0x800, source, 3);
        check(result.output == std::vector<std::uint8_t>({0xA5, 0x5A, 0x11}));
        check(result.consumed == 7 && result.state[8] == 0x1007 && result.state[9] == 0xFF1003);
        check(result.state[15] == 0xFFFF04 && result.state[16] == 0x800 && result.state[17] == 0x2704);
        check(result.stack_writes.size() == 16);
        check(result.stack_writes[0].address == 0xFFFEFE && result.stack_writes[0].value == 0);
        check(result.stack_writes[1].value == 10 && result.stack_writes[2].value == 0x12);
        check(result.stack_writes[15].address == 0xFFFEF1 && result.stack_writes[15].value == 0x34);
        const std::array<std::uint8_t, 7> format_b{0, 0, 0, 6, 0x55, 0, 0};
        const auto b = predict(entry, 0x800, format_b, 1);
        check(b.output == std::vector<std::uint8_t>({0x55}) && b.consumed == 7);
        check(b.stack_writes.size() == 28 && b.stack_writes[16].address == 0xFFFEEE);
        auto corrupted = result.state;
        corrupted[2] ^= 1;
        check(!compare_registers(result.state, corrupted).empty());
        corrupted = result.state; corrupted[17] ^= 1;
        check(!compare_registers(result.state, corrupted).empty());
        corrupted = result.state; corrupted[17] ^= 16;
        check(compare_registers(result.state, corrupted).empty()); // Explicitly unproven X, never authorizes override.
        auto bad_output = result.output; bad_output[1] ^= 1;
        check(compare_bytes(result.output, bad_output, "output").find("byte[1]") != std::string::npos);
        bool blocked = false;
        try { require_mode(Mode::NATIVE_OVERRIDE); } catch (const std::runtime_error&) { blocked = true; }
        check(blocked);
        require_mode(Mode::EMULATED); require_mode(Mode::SHADOW_NATIVE);
        std::cout << "hybrid contract and fail-closed override verified\n";
    } catch (const std::exception& error) { std::cerr << error.what(); return 1; }
}
