#include "tools/hybrid/runner_report.hpp"

namespace oasis::hybrid {

void write_runner_report_details(std::ostream& report, const Registry* registry,
                                 const BasicBlockRegistry* blocks,
                                 const MechanicalPrimitiveRegistry* primitives) {
    report << ",\n\"per_target\":[";
    if (registry) {
        for (std::size_t i = 0; i < registry->targets().size(); ++i) {
            const auto metrics = registry->targets()[i]->metrics();
            if (i) report << ',';
            report << "{\"target\":\"0x" << std::hex << registry->targets()[i]->target_address()
                   << std::dec << "\",\"natural_calls\":" << metrics.calls
                   << ",\"shadow_comparisons\":" << metrics.comparisons
                   << ",\"divergence_count\":" << metrics.divergences
                   << ",\"body_instruction_starts\":" << metrics.body_instructions
                   << ",\"native_override_calls\":" << metrics.override_calls
                   << ",\"fallback_emulated_calls\":" << (metrics.calls - metrics.override_calls)
                   << ",\"interrupts\":" << metrics.interrupts << '}';
        }
    }
    report << "]"
           << ",\n\"per_block\":[";
    if (blocks) {
        const auto metrics = blocks->metrics();
        for (std::size_t i = 0; i < metrics.per_block.size(); ++i) {
            if (i) report << ',';
            const auto& block = metrics.per_block[i];
            report << "{\"target\":\"0x" << std::hex << block.target << std::dec
                   << "\",\"end\":\"0x" << std::hex << block.end << std::dec
                   << "\",\"instruction_count\":" << block.instruction_count
                   << ",\"natural_entries\":" << block.natural_entries
                   << ",\"shadow_comparisons\":" << block.shadow_comparisons
                   << ",\"translated_entries\":" << block.translated_entries
                   << ",\"boundary_yields\":" << block.boundary_yields
                   << ",\"event_boundary_yields\":" << block.event_boundary_yields
                   << ",\"interrupt_boundary_yields\":" << block.interrupt_boundary_yields
                   << ",\"trace_boundary_yields\":" << block.trace_boundary_yields
                   << ",\"interrupted_resumptions\":" << block.interrupted_resumptions
                   << '}';
        }
    }
    report << "]"
           << ",\n\"mechanical_primitive\":{\"name\":\"MEMORY_CLEAR\",\"entry\":\"0x061266\",\"loop\":\"0x061268\",\"continuation\":\"0x06126C\"";
    if (primitives) {
        const auto metrics = primitives->metrics();
        report << ",\"invocations\":" << metrics.invocations
               << ",\"iterations\":" << metrics.iterations
               << ",\"dispatches\":" << metrics.dispatches
               << ",\"shadow_comparisons\":" << metrics.shadow_comparisons
               << ",\"divergences\":" << metrics.divergences
               << ",\"mid_operation_yields\":" << metrics.mid_operation_yields
               << ",\"resumptions\":" << metrics.resumptions
               << ",\"bytes_written\":" << metrics.bytes_written
               << ",\"min_address\":\"0x" << std::hex << metrics.min_address
               << "\",\"max_address\":\"0x" << metrics.max_address << std::dec << '"';
    }
    report << '}';
}

} // namespace oasis::hybrid
