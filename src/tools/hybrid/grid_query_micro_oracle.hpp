#pragma once

#include <array>
#include <iosfwd>
#include <string>
#include <vector>

namespace oasis::hybrid {

struct MicroCpuState {
    std::array<unsigned, 8> data{};
    std::array<unsigned, 8> address{};
    unsigned pc{}, sr{}, cycles{};
};

struct MicroStep {
    unsigned hook_address{}, register_pc{}, opcode{}, d0{}, d1{}, a6{}, a7{}, sr{};
};

struct MicroOracleResult {
    bool passed{};
    std::string failure;
    std::array<unsigned, 4> raw_stack{};
    unsigned bridge_stack_longword{}, rts_cpu_read_value{};
    MicroCpuState sentinel_state{};
    std::vector<MicroStep> steps;
};

struct MicroBridge {
    unsigned (*read_register)(unsigned){};
    int (*peek_memory)(unsigned){};
    void (*write_register)(unsigned, unsigned){};
    int (*write_memory)(unsigned, int, unsigned){};
    void (*run_cpu)(unsigned){};
    void (*stop_cpu)(){};
    void (*install_hook)(unsigned int (*)(int, int, unsigned, unsigned)){};
};

MicroOracleResult run_m68k_rts_micro_oracle(const MicroBridge& bridge);
void write_micro_oracle_json(std::ostream& out, const MicroOracleResult& result);

} // namespace oasis::hybrid
