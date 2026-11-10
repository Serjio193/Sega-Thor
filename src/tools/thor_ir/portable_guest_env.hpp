#ifndef OASIS_TOOLS_THOR_IR_PORTABLE_GUEST_ENV_HPP_
#define OASIS_TOOLS_THOR_IR_PORTABLE_GUEST_ENV_HPP_

#include "tools/thor_ir/ir_schema.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <vector>

namespace oasis::recompiler::ir {

inline constexpr std::uint32_t kBusAddressMask     = 0x00FFFFFFU;
inline constexpr std::uint32_t kCanonicalRomSize   = 0x00300000U; // 3 MiB (3,145,728 bytes)
inline constexpr std::uint32_t kCartridgeWindowMax = 0x00400000U; // 4 MiB Cartridge space
inline constexpr std::uint32_t kWorkRamBase        = 0x00FF0000U;
inline constexpr std::uint32_t kWorkRamSize        = 0x00010000U; // 64 KB

// CCR bit definitions
inline constexpr std::uint16_t kFlagC = 0x01U;
inline constexpr std::uint16_t kFlagV = 0x02U;
inline constexpr std::uint16_t kFlagZ = 0x04U;
inline constexpr std::uint16_t kFlagN = 0x08U;
inline constexpr std::uint16_t kFlagX = 0x10U;

// Status Register bit definitions
inline constexpr std::uint16_t kSrSupervisor = 0x2000U;
inline constexpr std::uint16_t kSrTrace      = 0x8000U;
inline constexpr std::uint16_t kSrIrqMask    = 0x0700U;

class PortableGuestEnvironment : public IGuestEnvironment {
public:
    using MmioHandler = std::function<void(std::uint32_t addr, IrOpSize size, std::uint32_t val, bool is_write)>;

    std::array<std::uint32_t, 8> d_regs{};
    std::array<std::uint32_t, 8> a_regs{};
    std::uint32_t usp{0x00FFFE00U};
    std::uint32_t ssp{0x00FFFF00U};
    std::uint32_t pc{0};
    std::uint16_t sr{0x2700U}; // Supervisor mode, IRQ level 7 masked

    std::vector<std::uint8_t> work_ram;
    std::vector<std::uint8_t> rom_data;
    std::size_t configured_rom_size{kCanonicalRomSize};
    std::map<std::uint32_t, std::uint8_t> extended_ram;
    MmioHandler mmio_handler{nullptr};

    int total_cycles{0};
    int cycle_budget{0};
    int sync_boundary_count{0};
    int pending_interrupt{0};

    bool bus_error{false};
    bool address_error{false};
    bool rom_write_violation{false};
    bool yielded{false};

    std::vector<std::uint32_t> sync_pcs;

    PortableGuestEnvironment() : work_ram(kWorkRamSize, 0) {
        a_regs[7] = ssp;
    }

    void set_rom(const std::vector<std::uint8_t>& rom) {
        rom_data = rom;
        configured_rom_size = rom_data.size();
    }

    void set_rom(const std::uint8_t* ptr, std::size_t size) {
        rom_data.assign(ptr, ptr + size);
        configured_rom_size = size;
    }

    void set_configured_rom_size(std::size_t sz) {
        configured_rom_size = sz;
    }

    std::size_t get_effective_rom_size() const noexcept {
        return (rom_data.size() > 0) ? rom_data.size() : configured_rom_size;
    }

    void set_sr(std::uint16_t new_sr) {
        const bool old_s = (sr & kSrSupervisor) != 0;
        const bool new_s = (new_sr & kSrSupervisor) != 0;
        if (old_s != new_s) {
            if (old_s) {
                ssp = a_regs[7];
                a_regs[7] = usp;
            } else {
                usp = a_regs[7];
                a_regs[7] = ssp;
            }
        }
        sr = new_sr;
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
        else if (reg.kind == IrRegKind::SR) set_sr(static_cast<std::uint16_t>(val));
        else if (reg.kind == IrRegKind::CCR) sr = (sr & 0xFFE0U) | (val & 0x1FU);
    }

    std::uint8_t read8_internal(std::uint32_t addr) {
        addr &= kBusAddressMask;
        // Mega Drive Main Work RAM: 0xE00000..0xFFFFFF (mirrored every 64KB)
        if (addr >= 0x00E00000U && addr <= 0x00FFFFFFU) {
            return work_ram[addr & 0xFFFFU];
        }
        const auto rom_sz = get_effective_rom_size();
        if (addr < rom_sz) {
            if (addr < rom_data.size()) {
                return rom_data[addr];
            }
            return 0U;
        }
        // Unmapped cartridge slot window up to 0xA00000
        if (addr >= rom_sz && addr < 0x00A00000U) {
            bus_error = true;
            return 0xFFU; // M68000 open bus pull-up
        }
        // Extended scratch RAM
        auto it = extended_ram.find(addr);
        if (it != extended_ram.end()) {
            return it->second;
        }
        // Unmapped peripheral or bus address
        bus_error = true;
        return 0xFFU;
    }

    void write8_internal(std::uint32_t addr, std::uint8_t val) {
        addr &= kBusAddressMask;
        // Mega Drive Main Work RAM: 0xE00000..0xFFFFFF (mirrored every 64KB)
        if (addr >= 0x00E00000U && addr <= 0x00FFFFFFU) {
            work_ram[addr & 0xFFFFU] = val;
            return;
        }
        const auto rom_sz = get_effective_rom_size();
        if (addr < rom_sz) {
            // ROM is strictly read-only: reject write and flag violation
            rom_write_violation = true;
            return;
        }
        if (addr >= rom_sz && addr < 0x00A00000U) {
            bus_error = true;
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

    bool irq_accepted(int level) const noexcept {
        return level == 7 || level > static_cast<int>((sr >> 8) & 7U);
    }

    bool try_deliver_interrupt(int level, std::uint32_t vector_pc) {
        if (!irq_accepted(level)) return false;
        enter_interrupt(level, vector_pc);
        return true;
    }

    void enter_interrupt(int level, std::uint32_t vector_pc) {
        const std::uint16_t old_sr = sr;
        set_sr(sr | kSrSupervisor);
        a_regs[7] = (a_regs[7] - 4U) & kBusAddressMask;
        write_memory(a_regs[7], IrOpSize::Long, pc);
        a_regs[7] = (a_regs[7] - 2U) & kBusAddressMask;
        write_memory(a_regs[7], IrOpSize::Word, old_sr);
        sr = (sr & ~kSrIrqMask) | ((static_cast<std::uint16_t>(level) & 7U) << 8U);
        pc = vector_pc & kBusAddressMask;
        total_cycles += 44;
    }

    void execute_rte() {
        const auto new_sr = static_cast<std::uint16_t>(read_memory(a_regs[7], IrOpSize::Word));
        a_regs[7] = (a_regs[7] + 2U) & kBusAddressMask;
        const auto new_pc = read_memory(a_regs[7], IrOpSize::Long);
        a_regs[7] = (a_regs[7] + 4U) & kBusAddressMask;
        set_sr(new_sr);
        pc = new_pc & kBusAddressMask;
        total_cycles += 20;
    }
};

} // namespace oasis::recompiler::ir

#endif // OASIS_TOOLS_THOR_IR_PORTABLE_GUEST_ENV_HPP_
