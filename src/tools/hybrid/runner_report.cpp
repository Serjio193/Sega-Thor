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
           << ",\n\"mechanical_primitive\":{\"name\":\"MECHANICAL_PRIMITIVE_FAMILY\"";
    if (primitives) {
        const auto metrics = primitives->metrics();
        report << ",\"invocations\":" << metrics.invocations
               << ",\"iterations\":" << metrics.iterations
               << ",\"dispatches\":" << metrics.dispatches
               << ",\"shadow_comparisons\":" << metrics.shadow_comparisons
               << ",\"divergences\":" << metrics.divergences
               << ",\"mid_operation_yields\":" << metrics.mid_operation_yields
               << ",\"boundary_yields\":" << metrics.boundary_yields
               << ",\"resumptions\":" << metrics.resumptions
               << ",\"reads\":" << metrics.reads
               << ",\"writes\":" << metrics.writes
               << ",\"hardware_accesses\":" << metrics.hardware_accesses
               << ",\"interrupt_events\":" << metrics.interrupt_events
               << ",\"candidates\":[";
        const auto contracts = primitives->contracts();
        const auto candidates = primitives->candidate_metrics();
        for (std::size_t i = 0; i < contracts.size(); ++i) {
            if (i) report << ',';
            const auto& contract = contracts[i];
            const auto& candidate = candidates[i];
            report << "{\"name\":\"" << contract.name
                   << "\",\"operation\":\""
                   << (contract.operation == MechanicalOperation::MEMORY_COPY ? "MEMORY_COPY" : "MEMORY_CLEAR")
                   << "\",\"body\":\"0x" << std::hex << contract.body_pc
                   << "\",\"loop\":\"0x" << contract.loop_pc
                   << "\",\"continuation\":\"0x" << contract.continuation_pc
                   << "\",\"body_opcode\":\"0x" << contract.body_opcode
                   << "\",\"dbf_opcode\":\"0x" << contract.dbf_opcode
                   << "\",\"dbf_displacement\":\"0x" << contract.dbf_displacement
                   << "\",\"width\":" << std::dec << contract.width
                   << ",\"counter_register\":" << contract.counter_register
                   << ",\"address_register\":" << contract.address_register
                   << ",\"source_register\":" << contract.source_register
                   << ",\"destination_register\":" << contract.destination_register
                   << ",\"invocations\":" << candidate.invocations
                   << ",\"iterations\":" << candidate.iterations
                   << ",\"dispatches\":" << candidate.dispatches
                   << ",\"shadow_comparisons\":" << candidate.shadow_comparisons
                   << ",\"divergences\":" << candidate.divergences
                   << ",\"mid_operation_yields\":" << candidate.mid_operation_yields
                   << ",\"boundary_yields\":" << candidate.boundary_yields
                   << ",\"resumptions\":" << candidate.resumptions
                   << ",\"reads\":" << candidate.reads
                   << ",\"writes\":" << candidate.writes
                   << ",\"hardware_accesses\":" << candidate.hardware_accesses
                   << ",\"interrupt_events\":" << candidate.interrupt_events
                   << ",\"initial_counter_min\":\"0x" << std::hex << candidate.initial_counter_min
                   << "\",\"initial_counter_max\":\"0x" << candidate.initial_counter_max
                   << "\",\"min_source_address\":\"0x" << candidate.min_source_address
                   << "\",\"max_source_address\":\"0x" << candidate.max_source_address
                   << "\",\"min_destination_address\":\"0x" << candidate.min_destination_address
                   << "\",\"max_destination_address\":\"0x" << candidate.max_destination_address
                   << "\",\"min_address\":\"0x" << candidate.min_address
                   << "\",\"max_address\":\"0x" << candidate.max_address << std::dec << "\"}";
        }
        report << ']';
    }
    report << '}';
}

} // namespace oasis::hybrid
