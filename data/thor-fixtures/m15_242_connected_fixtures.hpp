#ifndef OASIS_DATA_THOR_FIXTURES_M15_242_CONNECTED_FIXTURES_HPP_
#define OASIS_DATA_THOR_FIXTURES_M15_242_CONNECTED_FIXTURES_HPP_

#include <cstdint>
#include <string>
#include <vector>

namespace oasis::recompiler::fixtures {

struct ConnectedRamWrite {
    std::uint32_t addr{0};
    std::uint8_t val{0};
};

struct ConnectedFixture {
    const char* test_id;
    std::uint32_t caller_pc;
    std::uint32_t callee_pc;
    std::uint32_t expected_exit_pc;
    std::uint32_t d0;
    std::uint32_t a0;
    std::uint32_t a6;
    std::uint32_t a7;
    std::uint8_t flag_36;
    std::uint16_t scroll_a;
    std::uint16_t scroll_b;
    std::uint8_t ff1654;
    std::uint16_t expected_ccr;
    int expected_cycles;
    std::vector<ConnectedRamWrite> expected_writes;
};

inline const std::vector<ConnectedFixture>& get_connected_fixtures() {
    static const std::vector<ConnectedFixture> fixtures = {
        { "connected_path1_fast_exit_both", 0x00D2B4U, 0x00D344U, 0x02CAD8U,
          0x00000000U, 0x00000000U, 0x00FF8000U, 0x00FFFE00U,
          0x00U, 0x0000U, 0x0000U, 0x00U,
          0x0004U, 94, { { 0x00FF8036U, 0x00U }, { 0x00FF8036U, 0x00U } } },
        { "connected_path2_plane_a_single_line", 0x00D2B4U, 0x00D344U, 0x02CAD8U,
          0x00000000U, 0x00000000U, 0x00FF8000U, 0x00FFFE00U,
          0x01U, 0x4211U, 0x0000U, 0x00U,
          0x0005U, 192, { { 0x00FF8036U, 0x00U }, { 0x00FF1657U, 0x02U }, { 0x00FFA002U, 0x42U }, { 0x00FFA003U, 0x11U }, { 0x00FF8036U, 0x00U } } },
        { "connected_path3_plane_b_single_line", 0x00D2B4U, 0x00D344U, 0x02CAD8U,
          0x00000000U, 0x00000000U, 0x00FF8000U, 0x00FFFE00U,
          0x02U, 0x0000U, 0x8833U, 0x00U,
          0x0001U, 192, { { 0x00FF8036U, 0x02U }, { 0x00FF8036U, 0x00U }, { 0x00FF1657U, 0x04U }, { 0x00FFA402U, 0x88U }, { 0x00FFA403U, 0x33U } } },
        { "connected_path4_both_planes_single_line", 0x00D2B4U, 0x00D344U, 0x02CAD8U,
          0x00000000U, 0x00000000U, 0x00FF8000U, 0x00FFFE00U,
          0x03U, 0x1234U, 0x5678U, 0x00U,
          0x0001U, 290, { { 0x00FF8036U, 0x02U }, { 0x00FF1657U, 0x02U }, { 0x00FFA002U, 0x12U }, { 0x00FFA003U, 0x34U }, { 0x00FF8036U, 0x00U }, { 0x00FF1657U, 0x06U }, { 0x00FFA402U, 0x56U }, { 0x00FFA403U, 0x78U } } },
    };
    return fixtures;
}

} // namespace oasis::recompiler::fixtures

#endif // OASIS_DATA_THOR_FIXTURES_M15_242_CONNECTED_FIXTURES_HPP_
