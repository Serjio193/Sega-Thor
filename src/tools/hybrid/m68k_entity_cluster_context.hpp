#pragma once

#include "tools/hybrid/m68k_entity_cluster_types.hpp"

#include <cstdint>

namespace oasis::hybrid::entity_cluster {

class GuestExecutionContext {
public:
    explicit GuestExecutionContext(const M68k9D70StepApi& api) noexcept : api_(api) {}

    [[nodiscard]] std::uint32_t d(unsigned reg) const noexcept;
    void set_d(unsigned reg, std::uint32_t val) const noexcept;

    [[nodiscard]] std::uint32_t a(unsigned reg) const noexcept;
    void set_a(unsigned reg, std::uint32_t val) const noexcept;

    [[nodiscard]] std::uint32_t pc() const noexcept;
    void set_pc(std::uint32_t val) const noexcept;

    [[nodiscard]] std::uint16_t sr() const noexcept;
    void set_sr(std::uint16_t val) const noexcept;

    [[nodiscard]] std::uint8_t ccr() const noexcept;
    void set_ccr(std::uint8_t val) const noexcept;

    [[nodiscard]] bool read_u8(std::uint32_t addr, std::uint8_t& out) const noexcept;
    [[nodiscard]] bool read_u16(std::uint32_t addr, std::uint16_t& out) const noexcept;
    [[nodiscard]] bool read_u32(std::uint32_t addr, std::uint32_t& out) const noexcept;
    [[nodiscard]] bool read_s16(std::uint32_t addr, std::int16_t& out) const noexcept;

    [[nodiscard]] bool write_u8(std::uint32_t addr, std::uint8_t val) const noexcept;
    [[nodiscard]] bool write_u16(std::uint32_t addr, std::uint16_t val) const noexcept;
    [[nodiscard]] bool write_u32(std::uint32_t addr, std::uint32_t val) const noexcept;

    void add_cycles(int cycles) const noexcept;
    void skip_bus_refresh() const noexcept;

    bool begin_instruction(unsigned pc, std::uint16_t& opcode) const noexcept;
    bool finish_instruction(unsigned next_pc, std::uint16_t opcode, int cycle_adjust) const noexcept;

    [[nodiscard]] const M68k9D70StepApi& api() const noexcept { return api_; }

private:
    const M68k9D70StepApi& api_;
};

} // namespace oasis::hybrid::entity_cluster
