// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_blocks.hpp"

#include <vector>

namespace oasis::hybrid::generated {

const GeneratedBlockSpec kBlocks[] = {
    {0x002D66U, 0x002D7AU, 7U, execute_0x002D66, instruction_count_from_0x002D66},
    {0x0604BCU, 0x0604C2U, 1U, execute_0x0604BC, instruction_count_from_0x0604BC},
    {0x061032U, 0x061034U, 1U, execute_0x061032, instruction_count_from_0x061032},
    {0x03A85EU, 0x03A864U, 1U, execute_0x03A85E, instruction_count_from_0x03A85E},
    {0x03A8BAU, 0x03A8C0U, 1U, execute_0x03A8BA, instruction_count_from_0x03A8BA},
    {0x03A88CU, 0x03A892U, 1U, execute_0x03A88C, instruction_count_from_0x03A88C},
    {0x0003A0U, 0x0003A4U, 1U, execute_0x0003A0, instruction_count_from_0x0003A0},
    {0x03A8ACU, 0x03A8B0U, 1U, execute_0x03A8AC, instruction_count_from_0x03A8AC},
    {0x060312U, 0x060316U, 1U, execute_0x060312, instruction_count_from_0x060312},
    {0x002230U, 0x002234U, 1U, execute_0x002230, instruction_count_from_0x002230},
    {0x061360U, 0x061364U, 1U, execute_0x061360, instruction_count_from_0x061360},
    {0x003A0EU, 0x003A12U, 1U, execute_0x003A0E, instruction_count_from_0x003A0E},
    {0x0038A0U, 0x0038A4U, 1U, execute_0x0038A0, instruction_count_from_0x0038A0},
    {0x0003F2U, 0x0003F6U, 1U, execute_0x0003F2, instruction_count_from_0x0003F2},
    {0x003818U, 0x00381AU, 1U, execute_0x003818, instruction_count_from_0x003818},
    {0x0030BEU, 0x0030C0U, 1U, execute_0x0030BE, instruction_count_from_0x0030BE},
    {0x03A758U, 0x03A75AU, 1U, execute_0x03A758, instruction_count_from_0x03A758},
    {0x00D994U, 0x00D998U, 1U, execute_0x00D994, instruction_count_from_0x00D994},
    {0x003186U, 0x003188U, 1U, execute_0x003186, instruction_count_from_0x003186},
    {0x003250U, 0x003252U, 1U, execute_0x003250, instruction_count_from_0x003250},
    {0x061938U, 0x06193CU, 1U, execute_0x061938, instruction_count_from_0x061938},
    {0x002C18U, 0x002C1AU, 1U, execute_0x002C18, instruction_count_from_0x002C18},
    {0x0032EEU, 0x0032F6U, 2U, execute_0x0032EE, instruction_count_from_0x0032EE},
    {0x03A9ACU, 0x03A9B4U, 2U, execute_0x03A9AC, instruction_count_from_0x03A9AC},
    {0x03A9B4U, 0x03A9BCU, 2U, execute_0x03A9B4, instruction_count_from_0x03A9B4},
    {0x03A9CAU, 0x03A9D4U, 2U, execute_0x03A9CA, instruction_count_from_0x03A9CA},
    {0x000380U, 0x0003A0U, 16U, execute_0x000380, instruction_count_from_0x000380},
    {0x03A864U, 0x03A868U, 1U, execute_0x03A864, instruction_count_from_0x03A864},
    {0x03A7AEU, 0x03A7B8U, 2U, execute_0x03A7AE, instruction_count_from_0x03A7AE},
};

std::span<const GeneratedBlockSpec> blocks() {
    static const auto all = [] {
        std::vector<GeneratedBlockSpec> result(
            kBlocks, kBlocks + sizeof(kBlocks) / sizeof(kBlocks[0]));
        const auto m1145 = blocks_m1145();
        result.insert(result.end(), m1145.begin(), m1145.end());
        return result;
    }();
    return all;
}

std::span<const GeneratedBlockSpec> blocks_m1145() {
    static const auto all = [] {
        std::vector<GeneratedBlockSpec> result;
        const auto part0 = blocks_m1145_part0();
        const auto part1 = blocks_m1145_part1();
        const auto part2 = blocks_m1145_part2();
        const auto part3 = blocks_m1145_part3();
        result.reserve(part0.size() + part1.size() + part2.size() + part3.size());
        result.insert(result.end(), part0.begin(), part0.end());
        result.insert(result.end(), part1.begin(), part1.end());
        result.insert(result.end(), part2.begin(), part2.end());
        result.insert(result.end(), part3.begin(), part3.end());
        return result;
    }();
    return all;
}

} // namespace oasis::hybrid::generated
