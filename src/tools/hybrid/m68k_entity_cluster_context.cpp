#include "tools/hybrid/m68k_entity_cluster_context.hpp"

namespace oasis::hybrid::entity_cluster {
namespace {

constexpr unsigned kMask24 = 0xFFFFFFU;

} // namespace

std::uint32_t GuestExecutionContext::d(unsigned reg) const noexcept {
    if (reg >= 8U || !api_.read_register) return 0U;
    return api_.read_register(api_.context, reg);
}

void GuestExecutionContext::set_d(unsigned reg, std::uint32_t val) const noexcept {
    if (reg < 8U && api_.write_register) {
        api_.write_register(api_.context, reg, val);
    }
}

std::uint32_t GuestExecutionContext::a(unsigned reg) const noexcept {
    if (reg >= 8U || !api_.read_register) return 0U;
    return api_.read_register(api_.context, reg + 8U);
}

void GuestExecutionContext::set_a(unsigned reg, std::uint32_t val) const noexcept {
    if (reg < 8U && api_.write_register) {
        api_.write_register(api_.context, reg + 8U, val);
    }
}

std::uint32_t GuestExecutionContext::pc() const noexcept {
    if (!api_.read_register) return 0U;
    return api_.read_register(api_.context, 16U) & kMask24;
}

void GuestExecutionContext::set_pc(std::uint32_t val) const noexcept {
    if (api_.write_register) {
        api_.write_register(api_.context, 16U, val & kMask24);
    }
}

std::uint16_t GuestExecutionContext::sr() const noexcept {
    if (!api_.read_register) return 0U;
    return static_cast<std::uint16_t>(api_.read_register(api_.context, 17U));
}

void GuestExecutionContext::set_sr(std::uint16_t val) const noexcept {
    if (api_.write_register) {
        api_.write_register(api_.context, 17U, val);
    }
}

std::uint8_t GuestExecutionContext::ccr() const noexcept {
    return static_cast<std::uint8_t>(sr() & 0xFFU);
}

void GuestExecutionContext::set_ccr(std::uint8_t val) const noexcept {
    const std::uint16_t current = sr();
    set_sr(static_cast<std::uint16_t>((current & 0xFF00U) | val));
}

bool GuestExecutionContext::read_u8(std::uint32_t addr, std::uint8_t& out) const noexcept {
    if (!api_.read) return false;
    out = static_cast<std::uint8_t>(api_.read(api_.context, addr & kMask24, 1));
    return true;
}

bool GuestExecutionContext::read_u16(std::uint32_t addr, std::uint16_t& out) const noexcept {
    if (!api_.read) return false;
    out = static_cast<std::uint16_t>(api_.read(api_.context, addr & kMask24, 2));
    return true;
}

bool GuestExecutionContext::read_u32(std::uint32_t addr, std::uint32_t& out) const noexcept {
    if (!api_.read) return false;
    out = api_.read(api_.context, addr & kMask24, 4);
    return true;
}

bool GuestExecutionContext::read_s16(std::uint32_t addr, std::int16_t& out) const noexcept {
    std::uint16_t raw{};
    if (!read_u16(addr, raw)) return false;
    out = static_cast<std::int16_t>(raw);
    return true;
}

bool GuestExecutionContext::write_u8(std::uint32_t addr, std::uint8_t val) const noexcept {
    if (!api_.write) return false;
    api_.write(api_.context, addr & kMask24, 1, val);
    return true;
}

bool GuestExecutionContext::write_u16(std::uint32_t addr, std::uint16_t val) const noexcept {
    if (!api_.write) return false;
    api_.write(api_.context, addr & kMask24, 2, val);
    return true;
}

bool GuestExecutionContext::write_u32(std::uint32_t addr, std::uint32_t val) const noexcept {
    if (!api_.write) return false;
    api_.write(api_.context, addr & kMask24, 4, val);
    return true;
}

void GuestExecutionContext::add_cycles(int cycles) const noexcept {
    if (api_.add_cycles) {
        api_.add_cycles(api_.context, cycles);
    }
}

void GuestExecutionContext::skip_bus_refresh() const noexcept {
    if (api_.skip_bus_refresh) {
        api_.skip_bus_refresh(api_.context);
    }
}

bool GuestExecutionContext::begin_instruction(unsigned pc, std::uint16_t& opcode) const noexcept {
    if (api_.begin_instruction) api_.begin_instruction(api_.context, pc);
    if (!api_.fetch16) return false;
    opcode = static_cast<std::uint16_t>(api_.fetch16(api_.context));
    return true;
}

bool GuestExecutionContext::finish_instruction(unsigned next_pc, std::uint16_t opcode,
                                               int cycle_adjust) const noexcept {
    if (api_.observe_instruction) {
        api_.observe_instruction(api_.context, pc(), opcode);
    }
    if (api_.add_cycles && cycle_adjust != 0) {
        api_.add_cycles(api_.context, cycle_adjust);
    }
    set_pc(next_pc);
    if (api_.finish_instruction) {
        api_.finish_instruction(api_.context, next_pc);
    }
    return true;
}

} // namespace oasis::hybrid::entity_cluster
