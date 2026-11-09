#ifndef OASIS_TOOLS_THOR_IR_PORTABLE_GUEST_ENV_HPP_
#define OASIS_TOOLS_THOR_IR_PORTABLE_GUEST_ENV_HPP_

#include "tools/thor_ir/ir_schema.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <vector>

namespace oasis::recompiler::ir {

inline constexpr std::uint32_t kBusAddressMask = 0x00FFFFFFU;
inline constexpr std::uint32_t kWorkRamBase    = 0x00FF0000U;
inline constexpr std::uint32_t kWorkRamSize    = 0x00010000U; // 64 KB

// CCR bit definitions
inline constexpr std::uint16_t kFlagC = 0x01U;
inline constexpr std::uint16_t kFlagV = 0x02U;
inline constexpr std::uint16_t kFlagZ = 0x04U;
inline constexpr std::uint16_t kFlagN = 0x08U;
inline constexpr std::uint16_t kFlagX = 0x10U;

class PortableGuestEnvironment : public IGuestEnvironment {
public:
    using MmioHandler = std::function<void(std::uint32_t addr, IrOpSize size, std::uint32_t val, bool is_write)>;

    std::array<std::uint32_t, 8> d_regs{};
    std::array<std::uint32_t, 8> a_regs{};
    std::uint32_t pc{0};
    std::uint16_t sr{0x2700U}; // Supervisor mode, IRQ masked by default
    std::vector<std::uint8_t> work_ram;
    std::vector<std::uint8_t> rom_data;
    std::map<std::uint32_t, std::uint8_t> extended_ram;
    MmioHandler mmio_handler{nullptr};

    int total_cycles{0};
    int sync_boundary_count{0};
    bool bus_error{false};
    bool address_error{false};
    std::vector<std::uint32_t> sync_pcs;

    PortableGuestEnvironment() : work_ram(kWorkRamSize, 0) {}

    void set_rom(const std::vector<std::uint8_t>& rom) { rom_data = rom; }
    void set_rom(const std::uint8_t* ptr, std::size_t size) {
        rom_data.assign(ptr, ptr + size);
    }

    std::uint32_t read_register(IrRegister reg) override {
        if (reg.kind == IrRegKind::Data) return d_regs[reg.index & 7];
        if (reg.kind == IrRegKind::Address) return a_regs[reg.index & 7] & kBusAddressMask;
        if (reg.kind == IrRegKind::PC) return pc & kBusAddressMask;
        if (reg.kind == IrRegKind::SR) return sr;
        if (reg.kind == IrRegKind::CCR) return sr & 0x1FU;
        return 0;
    }

    void write_register(IrRegister reg, std::uint32_t val) override {
        if (reg.kind == IrRegKind::Data) d_regs[reg.index & 7] = val;
        else if (reg.kind == IrRegKind::Address) a_regs[reg.index & 7] = val & kBusAddressMask;
        else if (reg.kind == IrRegKind::PC) pc = val & kBusAddressMask;
        else if (reg.kind == IrRegKind::SR) sr = static_cast<std::uint16_t>(val);
        else if (reg.kind == IrRegKind::CCR) sr = (sr & 0xFFE0U) | (val & 0x1FU);
    }

    std::uint8_t read8_internal(std::uint32_t addr) {
        addr &= kBusAddressMask;
        if (addr >= kWorkRamBase && addr < kWorkRamBase + kWorkRamSize) {
            return work_ram[addr - kWorkRamBase];
        }
        if (addr < rom_data.size()) {
            return rom_data[addr];
        }
        auto it = extended_ram.find(addr);
        return (it != extended_ram.end()) ? it->second : 0U;
    }

    void write8_internal(std::uint32_t addr, std::uint8_t val) {
        addr &= kBusAddressMask;
        if (addr >= kWorkRamBase && addr < kWorkRamBase + kWorkRamSize) {
            work_ram[addr - kWorkRamBase] = val;
            return;
        }
        extended_ram[addr] = val;
    }

    std::uint32_t read_memory(std::uint32_t addr, IrOpSize size) override {
        addr &= kBusAddressMask;
        if ((size == IrOpSize::Word || size == IrOpSize::Long) && (addr & 1U) != 0) {
            address_error = true;
            return 0;
        }
        if (mmio_handler && (addr >= 0x00A00000U && addr < 0x00D00000U)) {
            mmio_handler(addr, size, 0, false);
        }
        if (size == IrOpSize::Byte) {
            return read8_internal(addr);
        } else if (size == IrOpSize::Word) {
            return (static_cast<std::uint32_t>(read8_internal(addr)) << 8U) |
                    static_cast<std::uint32_t>(read8_internal(addr + 1U));
        } else {
            return (static_cast<std::uint32_t>(read8_internal(addr)) << 24U) |
                   (static_cast<std::uint32_t>(read8_internal(addr + 1U)) << 16U) |
                   (static_cast<std::uint32_t>(read8_internal(addr + 2U)) << 8U) |
                    static_cast<std::uint32_t>(read8_internal(addr + 3U));
        }
    }

    void write_memory(std::uint32_t addr, IrOpSize size, std::uint32_t val) override {
        addr &= kBusAddressMask;
        if ((size == IrOpSize::Word || size == IrOpSize::Long) && (addr & 1U) != 0) {
            address_error = true;
            return;
        }
        if (mmio_handler && (addr >= 0x00A00000U && addr < 0x00D00000U)) {
            mmio_handler(addr, size, val, true);
        }
        if (size == IrOpSize::Byte) {
            write8_internal(addr, static_cast<std::uint8_t>(val & 0xFFU));
        } else if (size == IrOpSize::Word) {
            write8_internal(addr, static_cast<std::uint8_t>((val >> 8U) & 0xFFU));
            write8_internal(addr + 1U, static_cast<std::uint8_t>(val & 0xFFU));
        } else {
            write8_internal(addr, static_cast<std::uint8_t>((val >> 24U) & 0xFFU));
            write8_internal(addr + 1U, static_cast<std::uint8_t>((val >> 16U) & 0xFFU));
            write8_internal(addr + 2U, static_cast<std::uint8_t>((val >> 8U) & 0xFFU));
            write8_internal(addr + 3U, static_cast<std::uint8_t>(val & 0xFFU));
        }
    }

    std::uint16_t read_ccr() override { return sr & 0x1FU; }
    void write_ccr(std::uint16_t ccr) override { sr = (sr & 0xFFE0U) | (ccr & 0x1FU); }
    void add_cycles(int cycles) override { total_cycles += cycles; }
    void on_sync_boundary(std::uint32_t bpc) override {
        sync_boundary_count++;
        sync_pcs.push_back(bpc & kBusAddressMask);
    }
};

} // namespace oasis::recompiler::ir

#endif // OASIS_TOOLS_THOR_IR_PORTABLE_GUEST_ENV_HPP_
