#include "tools/hybrid/m68k_entity_cluster_coordinator.hpp"

#include "tools/hybrid/m68k_entity_cluster_context.hpp"
#include "tools/hybrid/m68k_entity_cluster_dispatch.hpp"
#include "tools/hybrid/m68k_entity_cluster_metrics.hpp"
#include "tools/hybrid/m68k_entity_cluster_state0.hpp"
#include "tools/hybrid/m68k_entity_cluster_target10.hpp"
#include "tools/hybrid/m68k_entity_cluster_target11_12.hpp"

namespace oasis::hybrid::entity_cluster {
namespace {

constexpr unsigned kMask24 = 0xFFFFFFU;

} // namespace

bool is_cluster_owned_pc(unsigned pc) noexcept {
    pc &= kMask24;
    return pc == kClusterDispatcherPc ||
           pc == kState0EntryPc ||
           pc == kTarget10EntryPc ||
           pc == 0x025822U ||
           pc == kTarget11EntryPc ||
           pc == kTarget12EntryPc;
}

M68k9D70StepResult step_entity_cluster(
    unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept {
    pc &= kMask24;
    if (!is_cluster_owned_pc(pc)) {
        return M68k9D70StepResult::NotHandled;
    }

    record_cluster_entry();
    if (!enabled) {
        return M68k9D70StepResult::NotHandled;
    }

    GuestExecutionContext ctx(api);
    ClusterTransfer transfer{};

    // 1. Dispatcher entry at 0x02575C
    if (pc == kClusterDispatcherPc) {
        record_dispatcher_entry();
        const auto disp_res = step_cluster_dispatcher(ctx, pc, transfer);
        if (disp_res != M68k9D70StepResult::InstructionStepped) {
            record_contract_fault();
            return disp_res;
        }
        record_stepped_instructions(4);

        // Chain into native state modules when destination is within native scope:
        const unsigned target_pc = transfer.next_pc & kMask24;
        if (target_pc == kState0EntryPc) {
            record_state0_entry();
            ClusterTransfer s0_transfer{};
            const auto s0_res = step_cluster_state0(ctx, target_pc, s0_transfer);
            if (s0_res == M68k9D70StepResult::InstructionStepped) {
                if (s0_transfer.kind == TransferKind::LoopbackJoin) {
                    record_loopback();
                    record_stepped_instructions(2);
                } else if (s0_transfer.kind == TransferKind::TerminalReturn) {
                    record_terminal_return();
                    record_stepped_instructions(3);
                }
            } else {
                record_contract_fault();
            }
            return s0_res;
        } else if (target_pc == kTarget10EntryPc) {
            record_target10_entry();
            ClusterTransfer t10_transfer{};
            const auto t10_res = step_cluster_target10(ctx, target_pc, t10_transfer);
            if (t10_res == M68k9D70StepResult::InstructionStepped) {
                if (t10_transfer.kind == TransferKind::ExternalRomChild) {
                    record_child_00f0d2();
                    record_stepped_instructions(10);
                } else if (t10_transfer.kind == TransferKind::TerminalReturn) {
                    record_terminal_return();
                    record_stepped_instructions(3);
                }
            } else {
                record_contract_fault();
            }
            return t10_res;
        } else if (target_pc == kTarget11EntryPc) {
            record_target11_entry();
            ClusterTransfer t11_transfer{};
            const auto t11_res = step_cluster_target11(ctx, target_pc, t11_transfer);
            if (t11_res == M68k9D70StepResult::InstructionStepped) {
                if (t11_transfer.kind == TransferKind::TerminalReturn) {
                    record_terminal_return();
                    record_stepped_instructions(6);
                } else {
                    record_stepped_instructions(10);
                }
            } else {
                record_contract_fault();
            }
            return t11_res;
        } else if (target_pc == kTarget12EntryPc) {
            record_target12_entry();
            ClusterTransfer t12_transfer{};
            const auto t12_res = step_cluster_target12(ctx, target_pc, t12_transfer);
            if (t12_res == M68k9D70StepResult::InstructionStepped) {
                if (t12_transfer.kind == TransferKind::LoopbackJoin) {
                    record_loopback();
                    record_stepped_instructions(4);
                } else {
                    record_terminal_return();
                    record_stepped_instructions(3);
                }
            } else {
                record_contract_fault();
            }
            return t12_res;
        }

        // Target is an unmigrated state handler (e.g. State 1, 2, 3..7, 8, 9)
        // Explicitly transfer execution to ROM interpreter at the exact target PC
        return M68k9D70StepResult::InstructionStepped;
    }

    // 2. Direct entry to State 0
    if (pc == kState0EntryPc) {
        record_state0_entry();
        const auto res = step_cluster_state0(ctx, pc, transfer);
        if (res == M68k9D70StepResult::InstructionStepped) {
            if (transfer.kind == TransferKind::LoopbackJoin) {
                record_loopback();
                record_stepped_instructions(2);
            } else if (transfer.kind == TransferKind::TerminalReturn) {
                record_terminal_return();
                record_stepped_instructions(3);
            }
        }
        return res;
    }

    // 3. Direct entry or continuation to Target 10
    if (pc == kTarget10EntryPc || pc == 0x025822U) {
        if (pc == kTarget10EntryPc) record_target10_entry();
        const auto res = step_cluster_target10(ctx, pc, transfer);
        if (res == M68k9D70StepResult::InstructionStepped) {
            if (transfer.kind == TransferKind::ExternalRomChild) {
                record_child_00f0d2();
                record_stepped_instructions(10);
            } else if (transfer.kind == TransferKind::TerminalReturn) {
                record_terminal_return();
                record_stepped_instructions(6);
            }
        }
        return res;
    }

    // 4. Direct entry to Target 11
    if (pc == kTarget11EntryPc) {
        record_target11_entry();
        const auto res = step_cluster_target11(ctx, pc, transfer);
        if (res == M68k9D70StepResult::InstructionStepped) {
            record_stepped_instructions(6);
        }
        return res;
    }

    // 5. Direct entry to Target 12
    if (pc == kTarget12EntryPc) {
        record_target12_entry();
        const auto res = step_cluster_target12(ctx, pc, transfer);
        if (res == M68k9D70StepResult::InstructionStepped) {
            record_stepped_instructions(3);
        }
        return res;
    }

    return M68k9D70StepResult::NotHandled;
}

} // namespace oasis::hybrid::entity_cluster
