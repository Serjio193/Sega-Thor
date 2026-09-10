#include "tools/re_slice_decoder.hpp"

#include <cassert>
#include <cstdint>
#include <vector>

int main() {
    using namespace oasis::tools;
    const std::vector<std::uint8_t> bytes{
        0x4B, 0xF9, 0x00, 0xFF, 0x00, 0x1A, // lea.l ($FF001A).L,A5
        0x4A, 0x2D, 0x00, 0x05,             // tst.b 5(A5)
        0x51, 0xF9, 0x00, 0xFF, 0x00, 0x10, // sf.b ($FF0010).L
        0x4E, 0x75,
    };
    const auto slice = decode_m68k_slice(bytes, {.entry = 0, .byte_budget = bytes.size()});
    assert(slice.instructions.size() == 4U);

    const auto& base = slice.instructions[0];
    assert(base.memory_references.size() == 1U);
    assert(base.memory_references.front().address == 0x00FF001AU);
    assert(base.memory_references.front().access == MemoryAccess::address);

    const auto& derived = slice.instructions[1];
    assert(derived.memory_references.empty());
    assert(derived.unresolved_memory_references.size() == 1U);
    assert(derived.unresolved_memory_references.front().mode == 5U);
    assert(derived.unresolved_memory_references.front().register_index == 5U);

    const auto& fixed = slice.instructions[2];
    assert(fixed.memory_references.size() == 1U);
    assert(fixed.memory_references.front().address == 0x00FF0010U);
    assert(fixed.memory_references.front().width_bytes == 1U);
    assert(fixed.memory_references.front().access == MemoryAccess::write);
    return 0;
}
