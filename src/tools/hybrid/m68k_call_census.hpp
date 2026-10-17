#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace oasis::hybrid::call_census {

using RegisterRead = unsigned (*)(unsigned);
using MemoryPeek = int (*)(unsigned);

struct CpuState {
    std::array<unsigned, 8> d{};
    std::array<unsigned, 8> a{};
    unsigned pc{};
    unsigned sr{};
};

struct Transfer {
    unsigned source_pc{};
    unsigned target_pc{};
    unsigned return_pc{};
    std::string kind;
    unsigned opcode{};
};

[[nodiscard]] std::string format_hex(unsigned value, unsigned width = 8);
[[nodiscard]] bool decode_transfer(unsigned source_pc, unsigned target_pc,
                                    const CpuState& target_state,
                                    unsigned opcode, unsigned extension,
                                    unsigned long_extension, Transfer& out);

class M68kCallCensus {
public:
    void set_rom(const std::uint8_t* rom, std::size_t rom_size) noexcept;
    void on_hook(unsigned type, int width, unsigned address, unsigned value,
                 unsigned previous_pc, unsigned frame, RegisterRead read_register,
                 MemoryPeek peek_memory);
    void execute(unsigned pc, unsigned previous_pc, unsigned frame,
                 const CpuState& state, unsigned current_opcode,
                 unsigned source_opcode, unsigned source_extension,
                 unsigned source_long_extension, unsigned return_address);
    void memory(unsigned type, unsigned pc, unsigned address,
                unsigned width, unsigned value);
    void write(const std::filesystem::path& census_path,
               const std::filesystem::path& captures_path) const;
    [[nodiscard]] std::size_t target_count() const noexcept;
    [[nodiscard]] std::size_t full_capture_count() const noexcept;
    [[nodiscard]] bool has_active() const noexcept;

private:
    struct Stats {
        std::size_t calls{};
        std::size_t stack_return_matches{};
        std::size_t stack_return_mismatches{};
        std::map<unsigned, std::size_t> callers;
        std::map<unsigned, std::size_t> selected_caller_frame_calls;
        std::map<std::string, std::size_t> kinds;
        unsigned first_frame{};
        unsigned last_frame{};
        std::array<std::set<unsigned>, 16> register_values;
        std::set<unsigned> sr_values;
        std::size_t ram_writes{};
        std::size_t rom_reads{};
        std::size_t completed_returns{};
        std::size_t full_captures{};
    };
    struct MemoryEvent { unsigned type{}, pc{}, address{}, width{}, value{}; };
    struct Active {
        Transfer transfer;
        unsigned frame{};
        unsigned entry_sp{};
        CpuState entry;
        CpuState rts_pre;
        CpuState exit;
        bool saw_rts{};
        bool nested_call{};
        bool indirect_control{};
        bool overflow{};
        std::size_t instructions{};
        std::vector<unsigned> pc_path;
        std::vector<MemoryEvent> events;
    };

    std::map<unsigned, Stats> stats_;
    std::vector<Active> active_;
    std::vector<Active> captures_;
    const std::uint8_t* rom_{};
    std::size_t rom_size_{};
};

} // namespace oasis::hybrid::call_census
