#ifndef OASIS_DATA_THOR_FIXTURES_M15_241_CPU_REFERENCE_FIXTURES_HPP_
#define OASIS_DATA_THOR_FIXTURES_M15_241_CPU_REFERENCE_FIXTURES_HPP_

#include <cstdint>
#include <string_view>
#include <vector>

namespace oasis::recompiler::fixtures {

inline constexpr std::string_view kReferenceCoreHash = "ca4be3049e56b1fdfba9a7c727bebbdc645b3d50203d635a468048ba81165134";

struct RamInit { std::uint32_t addr; std::uint8_t val; };
struct RamWrite { std::uint32_t addr; std::uint8_t val; };

struct CpuFixture {
    const char* test_id;
    std::uint32_t entry_pc;
    std::uint32_t d0, d2, a6, a7;
    std::vector<RamInit> initial_ram;
    std::uint32_t expected_exit_pc;
    std::uint32_t exp_d0, exp_d2, exp_a6, exp_a7;
    std::uint16_t exp_ccr;
    int expected_cycles;
    std::vector<RamWrite> expected_writes;
};

inline const std::vector<CpuFixture>& get_cpu_fixtures() {
    static const std::vector<CpuFixture> fixtures = {
        {
            "00e682_path1_normal_unclamped",
            0x00E682U,
            0x00000000U, 0x00000000U, 0x00FF8000U, 0x00FFFE00U,
            {{0x00FF8036U, 0x40U}, {0x00FF809DU, 0x00U}, {0x00FF8082U, 0x00U}, },
            0x00E6A8U,
            0x00000000U, 0x00000000U, 0x00FF8000U, 0x00FFFE00U,
            0x04U,
            104,
            {{0x00FF8036U, 0x00U}, {0x00FF8036U, 0x01U}, }
        },
        {
            "00e682_path2_flag_9d_nonzero",
            0x00E682U,
            0x00000000U, 0x00000000U, 0x00FF8000U, 0x00FFFE00U,
            {{0x00FF8036U, 0x40U}, {0x00FF809DU, 0x01U}, {0x00FF8082U, 0x00U}, },
            0x00E6AAU,
            0x00000001U, 0x00000000U, 0x00FF8000U, 0x00FFFE00U,
            0x00U,
            30,
            {{0x00FF8036U, 0x00U}, }
        },
        {
            "00e682_path3_flag_82_nonzero",
            0x00E682U,
            0x00000000U, 0x00000000U, 0x00FF8000U, 0x00FFFE00U,
            {{0x00FF8036U, 0x00U}, {0x00FF809DU, 0x00U}, {0x00FF8082U, 0x80U}, },
            0x00E6A8U,
            0x00000080U, 0x00000000U, 0x00FF8000U, 0x00FFFE00U,
            0x08U,
            64,
            {{0x00FF8036U, 0x00U}, }
        },
        {
            "00e682_path4_d2_nonzero",
            0x00E682U,
            0x00000000U, 0x00000005U, 0x00FF8000U, 0x00FFFE00U,
            {{0x00FF8036U, 0x40U}, {0x00FF809DU, 0x00U}, {0x00FF8082U, 0x00U}, },
            0x00E6A8U,
            0x00000000U, 0x00000005U, 0x00FF8000U, 0x00FFFE00U,
            0x00U,
            78,
            {{0x00FF8036U, 0x00U}, }
        },
        {
            "00e682_path5_d0_nonzero",
            0x00E682U,
            0x000000FFU, 0x00000000U, 0x00FF8000U, 0x00FFFE00U,
            {{0x00FF8036U, 0x40U}, {0x00FF809DU, 0x00U}, {0x00FF8082U, 0x00U}, },
            0x00E6A8U,
            0x00000000U, 0x00000000U, 0x00FF8000U, 0x00FFFE00U,
            0x04U,
            104,
            {{0x00FF8036U, 0x00U}, {0x00FF8036U, 0x01U}, }
        },
        {
            "016294_path1_flag_set_branch_taken",
            0x016294U,
            0x00000000U, 0x00000000U, 0x00FFA000U, 0x00FFFE00U,
            {{0x00FF0E1DU, 0x01U}, },
            0x0163A4U,
            0x00000000U, 0x00000000U, 0x00FFA000U, 0x00FFFE00U,
            0x00U,
            22,
            {}
        },
        {
            "016294_path2_timer_increment_bcs",
            0x016294U,
            0x00000000U, 0x00000000U, 0x00FFA000U, 0x00FFFE00U,
            {{0x00FF0E1DU, 0x00U}, {0x00FFA0B0U, 0x00U}, {0x00FFA0B1U, 0x2DU}, },
            0x015AFCU,
            0x0000002EU, 0x00000000U, 0x00FFA000U, 0x00FFFE00U,
            0x09U,
            64,
            {{0x00FFA0B0U, 0x00U}, {0x00FFA0B1U, 0x2EU}, }
        },
        {
            "006e7a_path1_bit4_set_bne_taken",
            0x006E7AU,
            0x00000000U, 0x00000000U, 0x00000000U, 0x00FFFE00U,
            {{0x00FF165FU, 0x10U}, },
            0x006E86U,
            0x00000000U, 0x00000000U, 0x00000000U, 0x00FFFE00U,
            0x00U,
            22,
            {}
        },
        {
            "006e7a_path2_bit4_clear_rts",
            0x006E7AU,
            0x00000000U, 0x00000000U, 0x00000000U, 0x00FFFE00U,
            {{0x00FF165FU, 0x00U}, },
            0x006E84U,
            0x00000000U, 0x00000000U, 0x00000000U, 0x00FFFE00U,
            0x04U,
            36,
            {}
        },
    };
    return fixtures;
}

} // namespace oasis::recompiler::fixtures

#endif // OASIS_DATA_THOR_FIXTURES_M15_241_CPU_REFERENCE_FIXTURES_HPP_
