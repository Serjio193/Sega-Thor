#include "tools/re_cfg_closure.hpp"

#include <cassert>
#include <cstdint>
#include <vector>

int main() {
    using namespace oasis::tools;
    const std::vector<std::uint8_t> closed{
        0x60, 0x02, // BRA.S to the RTS at 4
        0x4E, 0x71, // unreachable NOP is not absorbed into the object
        0x4E, 0x75,
    };
    const std::vector<std::uint32_t> seed{0};
    const auto result = decode_closed_cfg(closed, 0, 6, seed);
    assert(result.closed);
    assert(result.blockers.empty());
    assert(result.instructions.size() == 2);
    assert(result.decoded_bytes == 4);

    const std::vector<std::uint8_t> indirect{
        0x4E, 0xD0, // JMP (A0)
        0x4E, 0x75,
    };
    const auto unresolved = decode_closed_cfg(indirect, 0, 4, seed);
    assert(!unresolved.closed);
    assert(unresolved.blockers.size() == 1);
    assert(unresolved.blockers.front() == "UNRESOLVED_INDIRECT_TARGET");

    const std::vector<std::uint8_t> escapes{
        0x4E, 0x71, // fallthrough leaves the candidate extent without known target
    };
    const auto open = decode_closed_cfg(escapes, 0, 2, seed);
    assert(!open.closed);
    assert(open.blockers.front() == "CFG_ESCAPES_UNKNOWN_WITHOUT_EXACT_TARGET");
    return 0;
}
