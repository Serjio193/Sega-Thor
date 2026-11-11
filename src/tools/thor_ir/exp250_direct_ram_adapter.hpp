#pragma once

#include "tools/thor_ir/exp250_dispatch_hybrid_adapter.hpp"
#include "tools/thor_ir/exp250_fused_hybrid_adapter.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_mode_b.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_router.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace oasis::recompiler::ir {

struct Exp250MemoryProfileMetrics {
    std::uint64_t reads_byte{0};
    std::uint64_t reads_word{0};
    std::uint64_t reads_long{0};
    std::uint64_t writes_byte{0};
    std::uint64_t writes_word{0};
    std::uint64_t writes_long{0};

    std::uint64_t ram_canonical_accesses{0}; // 0xFF0000 - 0xFFFFFF
    std::uint64_t ram_mirror_accesses{0};    // 0xE00000 - 0xFEFFFF
    std::uint64_t rom_accesses{0};           // 0x000000 - 0x3FFFFF
    std::uint64_t vdp_accesses{0};           // 0xC00000 - 0xDFFFFF
    std::uint64_t mmio_accesses{0};          // 0xA00000 - 0xBFFFFF
    std::uint64_t other_accesses{0};

    std::uint64_t fast_eligible_reads{0};
    std::uint64_t fast_eligible_writes{0};
    std::uint64_t unaligned_word_long{0};
    std::uint64_t boundary_crossing{0};

    std::uint64_t fast_ram_reads_byte{0};
    std::uint64_t fast_ram_reads_word{0};
    std::uint64_t fast_ram_reads_long{0};
    std::uint64_t fast_ram_writes_byte{0};
    std::uint64_t fast_ram_writes_word{0};
    std::uint64_t fast_ram_writes_long{0};
    std::uint64_t fallback_bus_reads{0};
    std::uint64_t fallback_bus_writes{0};

    std::array<std::uint32_t, 8> top_addr_keys{};
    std::array<std::uint64_t, 8> top_addr_counts{};
};

namespace detail {
inline std::uint8_t*& direct_ram_ptr_mut() noexcept {
    static std::uint8_t* ptr{nullptr};
    return ptr;
}
inline std::size_t& direct_ram_size_mut() noexcept {
    static std::size_t sz{0x10000};
    return sz;
}
inline Exp250MemoryProfileMetrics& exp250_mem_metrics_mut() noexcept {
    static Exp250MemoryProfileMetrics m{};
    return m;
}
} // namespace detail

inline void set_m15250_direct_ram(std::uint8_t* ptr, std::size_t sz) noexcept {
    detail::direct_ram_ptr_mut() = ptr;
    detail::direct_ram_size_mut() = sz;
}

inline std::uint8_t* get_m15250_direct_ram() noexcept {
    return detail::direct_ram_ptr_mut();
}

inline const Exp250MemoryProfileMetrics& get_exp250_memory_profile_metrics() noexcept {
    return detail::exp250_mem_metrics_mut();
}

inline void reset_exp250_memory_profile_metrics() noexcept {
    detail::exp250_mem_metrics_mut() = {};
}

inline void record_mem_access(Exp250MemoryProfileMetrics& m, std::uint32_t addr, int width, bool is_write) noexcept {
    const std::uint32_t ma = addr & kBusAddressMask;
    if (is_write) {
        if (width == 1) ++m.writes_byte;
        else if (width == 2) ++m.writes_word;
        else if (width == 4) ++m.writes_long;
    } else {
        if (width == 1) ++m.reads_byte;
        else if (width == 2) ++m.reads_word;
        else if (width == 4) ++m.reads_long;
    }

    const bool in_canon = (ma >= 0xFF0000U && ma <= 0xFFFFFFU);
    const bool in_mirror = (ma >= 0xE00000U && ma < 0xFF0000U);
    if (in_canon) ++m.ram_canonical_accesses;
    else if (in_mirror) ++m.ram_mirror_accesses;
    else if (ma < 0x400000U) ++m.rom_accesses;
    else if ((ma & 0xE00000U) == 0xC00000U) ++m.vdp_accesses;
    else if ((ma & 0xFE0000U) == 0xA00000U) ++m.mmio_accesses;
    else ++m.other_accesses;

    const bool in_ram = in_canon || in_mirror;
    const bool aligned = (width == 1) || ((ma & 1U) == 0U);
    const std::uint32_t offset = ma & 0xFFFFU;
    const bool within_bounds = (offset + static_cast<std::uint32_t>(width) <= 0x10000U);

    if (!aligned) ++m.unaligned_word_long;
    if (in_ram && !within_bounds) ++m.boundary_crossing;

    if (in_ram && aligned && within_bounds) {
        if (is_write) ++m.fast_eligible_writes;
        else ++m.fast_eligible_reads;
    }

    for (std::size_t i = 0; i < m.top_addr_keys.size(); ++i) {
        if (m.top_addr_keys[i] == ma) {
            ++m.top_addr_counts[i];
            return;
        }
        if (m.top_addr_counts[i] == 0) {
            m.top_addr_keys[i] = ma;
            m.top_addr_counts[i] = 1;
            return;
        }
    }
}

class HybridM68kDirectRamBus250 final {
public:
    explicit HybridM68kDirectRamBus250(const oasis::hybrid::M68k9D70StepApi& api) noexcept
        : api_(api), direct_ram_(get_m15250_direct_ram()), ram_size_(detail::direct_ram_size_mut()) {}

    std::uint32_t d(int i) const noexcept { return api_.read_register(api_.context, i); }
    void set_d(int i, std::uint32_t v) noexcept { api_.write_register(api_.context, i, v); }
    std::uint32_t a(int i) const noexcept { return api_.read_register(api_.context, 8 + i); }
    void set_a(int i, std::uint32_t v) noexcept { api_.write_register(api_.context, 8 + i, v); }

    std::uint16_t ccr() const noexcept {
        return static_cast<std::uint16_t>(api_.read_register(api_.context, 17) & 0x1FU);
    }
    void set_ccr(std::uint16_t c) noexcept {
        const auto sr = api_.read_register(api_.context, 17);
        api_.write_register(api_.context, 17, (sr & ~0x1FU) | (c & 0x1FU));
    }

    bool check_align(std::uint32_t addr, IrOpSize sz) noexcept {
        if (sz != IrOpSize::Byte && (addr & 1U) != 0U) {
            address_fault_ = true;
            return false;
        }
        return true;
    }

    std::uint32_t read_m(std::uint32_t addr, IrOpSize sz) noexcept {
        const auto ma = addr & kBusAddressMask;
        const int w = (sz == IrOpSize::Byte) ? 1 : (sz == IrOpSize::Word ? 2 : 4);
        auto& mem_m = detail::exp250_mem_metrics_mut();
        record_mem_access(mem_m, ma, w, false);

        if (direct_ram_ && (ma >= 0xE00000U && ma <= 0xFFFFFFU)) {
            const std::uint32_t offset = ma & 0xFFFFU;
            if (offset + static_cast<std::uint32_t>(w) <= ram_size_) {
                if (w == 1) {
                    ++mem_m.fast_ram_reads_byte;
                    return direct_ram_[offset ^ 1U];
                }
                if ((offset & 1U) == 0U) {
                    if (w == 2) {
                        ++mem_m.fast_ram_reads_word;
                        return *reinterpret_cast<const std::uint16_t*>(direct_ram_ + offset);
                    }
                    if (w == 4) {
                        ++mem_m.fast_ram_reads_long;
                        const auto hi = *reinterpret_cast<const std::uint16_t*>(direct_ram_ + offset);
                        const auto lo = *reinterpret_cast<const std::uint16_t*>(direct_ram_ + offset + 2U);
                        return (static_cast<std::uint32_t>(hi) << 16) | lo;
                    }
                }
            }
        }
        ++mem_m.fallback_bus_reads;
        return api_.read(api_.context, ma, w);
    }

    void write_m(std::uint32_t addr, IrOpSize sz, std::uint32_t val) noexcept {
        const auto ma = addr & kBusAddressMask;
        const int w = (sz == IrOpSize::Byte) ? 1 : (sz == IrOpSize::Word ? 2 : 4);
        auto& mem_m = detail::exp250_mem_metrics_mut();
        record_mem_access(mem_m, ma, w, true);

        if (direct_ram_ && (ma >= 0xE00000U && ma <= 0xFFFFFFU)) {
            const std::uint32_t offset = ma & 0xFFFFU;
            if (offset + static_cast<std::uint32_t>(w) <= ram_size_) {
                if (w == 1) {
                    ++mem_m.fast_ram_writes_byte;
                    direct_ram_[offset ^ 1U] = static_cast<std::uint8_t>(val);
                    return;
                }
                if ((offset & 1U) == 0U) {
                    if (w == 2) {
                        ++mem_m.fast_ram_writes_word;
                        *reinterpret_cast<std::uint16_t*>(direct_ram_ + offset) = static_cast<std::uint16_t>(val);
                        return;
                    }
                    if (w == 4) {
                        ++mem_m.fast_ram_writes_long;
                        *reinterpret_cast<std::uint16_t*>(direct_ram_ + offset) = static_cast<std::uint16_t>(val >> 16);
                        *reinterpret_cast<std::uint16_t*>(direct_ram_ + offset + 2U) = static_cast<std::uint16_t>(val & 0xFFFFU);
                        return;
                    }
                }
            }
        }
        ++mem_m.fallback_bus_writes;
        api_.write(api_.context, ma, w, val);
    }

    bool faulted() const noexcept { return div_zero_ || address_fault_; }
    void fault_addr() noexcept { address_fault_ = true; }
    void fault_div0() noexcept { div_zero_ = true; }
    bool slice_yield() const noexcept { return slice_yield_; }

    bool begin_insn(std::uint32_t pc, std::uint16_t op) noexcept {
        if (step_count_ > 0 && api_.read_scheduler_field) {
            const int cur_cyc = api_.read_scheduler_field(api_.context, 0);
            const int end_cyc = api_.read_scheduler_field(api_.context, 1);
            const int int_lvl = api_.read_scheduler_field(api_.context, 8);
            const int int_msk = api_.read_scheduler_field(api_.context, 9);
            if (int_lvl > int_msk || (end_cyc > 0 && cur_cyc >= end_cyc)) {
                slice_yield_ = true;
                return false;
            }
        }
        last_insn_pc_ = pc & kBusAddressMask;
        last_insn_op_ = op;
        ++step_count_;
        if (api_.observe_instruction) api_.observe_instruction(api_.context, pc & kBusAddressMask, op);
        if (api_.begin_instruction) api_.begin_instruction(api_.context, op);
        return true;
    }

    bool end_insn(std::uint32_t next_pc, std::uint16_t op, int m68k_cyc, int adj) noexcept {
        const auto np = next_pc & kBusAddressMask;
        const int b0 = api_.peek(api_.context, np);
        const int b1 = api_.peek(api_.context, (np + 1U) & kBusAddressMask);
        if (b0 < 0 || b1 < 0) return false;
        const auto next_word = static_cast<unsigned>(((b0 & 0xFF) << 8) | (b1 & 0xFF));
        api_.set_return_state(api_.context, np, (np + 2U) & kBusAddressMask, next_word);

        last_next_pc_ = np;
        last_op_ = op;
        accumulated_cycles_ += m68k_cyc;
        if (adj != 0) {
            const int delta = (adj < 0)
                ? (api_.byte_not_taken_adjustment ? api_.byte_not_taken_adjustment(api_.context) : -14)
                : (api_.word_not_taken_adjustment ? api_.word_not_taken_adjustment(api_.context) : 14);
            if (delta != 0 && api_.add_cycles) api_.add_cycles(api_.context, delta);
        }
        if (api_.finish_instruction) api_.finish_instruction(api_.context, op);
        return true;
    }

    bool end_insn_mc(std::uint32_t next_pc, std::uint16_t op, int m68k_cyc, int hybrid_delta_mc, bool skip_refresh) noexcept {
        const auto np = next_pc & kBusAddressMask;
        const int b0 = api_.peek(api_.context, np);
        const int b1 = api_.peek(api_.context, (np + 1U) & kBusAddressMask);
        if (b0 < 0 || b1 < 0) return false;
        const auto next_word = static_cast<unsigned>(((b0 & 0xFF) << 8) | (b1 & 0xFF));
        api_.set_return_state(api_.context, np, (np + 2U) & kBusAddressMask, next_word);

        last_next_pc_ = np;
        last_op_ = op;
        accumulated_cycles_ += m68k_cyc;
        if (hybrid_delta_mc != 0 && api_.add_cycles) api_.add_cycles(api_.context, hybrid_delta_mc);
        if (skip_refresh && api_.skip_bus_refresh) api_.skip_bus_refresh(api_.context);
        if (api_.finish_instruction) api_.finish_instruction(api_.context, op);
        return true;
    }

    std::uint32_t step_count() const noexcept { return step_count_; }
    int accumulated_cycles() const noexcept { return accumulated_cycles_; }
    std::uint16_t last_op() const noexcept { return last_op_; }
    std::uint32_t last_next_pc() const noexcept { return last_next_pc_; }

private:
    const oasis::hybrid::M68k9D70StepApi& api_;
    std::uint8_t* direct_ram_{nullptr};
    std::size_t ram_size_{0x10000};
    std::uint32_t step_count_{0};
    int accumulated_cycles_{0};
    std::uint32_t last_insn_pc_{0};
    std::uint16_t last_insn_op_{0};
    std::uint32_t last_next_pc_{0};
    std::uint16_t last_op_{0};
    bool div_zero_{false};
    bool address_fault_{false};
    bool slice_yield_{false};
};

inline oasis::hybrid::M68k9D70StepResult step_m68k_portable_m15250_direct_ram(
    unsigned pc_in, const oasis::hybrid::M68k9D70StepApi& api, bool enabled) noexcept {
    const std::uint32_t pc = static_cast<std::uint32_t>(pc_in) & kBusAddressMask;
    auto& m = detail::exp250_metrics_mut();
    bool& in_port = detail::in_exp250_mut();

    if (!enabled) return oasis::hybrid::M68k9D70StepResult::NotHandled;

    if (!is_compiled_exp250_dispatch_pc(pc)) {
        if (in_port) {
            in_port = false;
            ++m.valid_uncompiled_exits;
        }
        return oasis::hybrid::M68k9D70StepResult::NotHandled;
    }

    if (!in_port) {
        in_port = true;
        ++m.m15250_entries;
    }

    HybridM68kDirectRamBus250 bus{api};
    std::uint32_t cur_pc = pc;
    std::uint32_t next_pc = pc;
    std::uint32_t fused_blocks = 0;
    std::uint32_t fused_steps = 0;

    while (true) {
        const auto prev_steps = bus.step_count();
        if (run_exp250_mode_b_all(bus, cur_pc, next_pc)) {
            ++fused_blocks;
            fused_steps += (bus.step_count() - prev_steps);
            if (!bus.slice_yield() && is_multiblock_candidate_edge(cur_pc, next_pc) && fused_blocks < 3) {
                cur_pc = next_pc;
                ++m.internal_cfg_edges_traversed;
                continue;
            }
            break;
        } else if (bus.slice_yield()) {
            fused_steps += (bus.step_count() - prev_steps);
            next_pc = bus.last_next_pc();
            break;
        } else if (step_one_exp250_dispatch_ir(bus, cur_pc, next_pc)) {
            fused_steps += (bus.step_count() - prev_steps);
            break;
        } else {
            break;
        }
    }

    if (fused_steps == 0) {
        if (bus.faulted()) {
            ++m.cpu_exceptions;
            ++m.original_cpu_fallbacks;
            in_port = false;
            return oasis::hybrid::M68k9D70StepResult::NotHandled;
        }
        ++m.contract_faults;
        in_port = false;
        return oasis::hybrid::M68k9D70StepResult::ContractFault;
    }

    m.m15250_guest_steps += fused_steps;
    m.disjoint_new_native_steps += fused_steps;
    m.fused_blocks += fused_blocks;

    if (fused_blocks >= 2) {
        ++m.multi_block_entries;
        m.multi_block_chained_blocks += fused_blocks;
    }

    if (bus.last_op() == 0x4E75U) ++m.completed_returns;
    if (!is_compiled_exp250_dispatch_pc(next_pc)) {
        in_port = false;
        ++m.valid_uncompiled_exits;
    }
    return oasis::hybrid::M68k9D70StepResult::InstructionStepped;
}

} // namespace oasis::recompiler::ir
