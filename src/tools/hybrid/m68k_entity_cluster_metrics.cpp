#include "tools/hybrid/m68k_entity_cluster_metrics.hpp"

#include <fstream>

namespace oasis::hybrid::entity_cluster {
namespace {

ClusterMetrics g_metrics{};

} // namespace

const ClusterMetrics& get_cluster_metrics() noexcept {
    return g_metrics;
}

void record_cluster_entry() noexcept {
    ++g_metrics.cluster_entries;
}

void record_dispatcher_entry() noexcept {
    ++g_metrics.dispatcher_entries;
}

void record_state0_entry() noexcept {
    ++g_metrics.state0_entries;
}

void record_target10_entry() noexcept {
    ++g_metrics.target10_entries;
}

void record_target11_entry() noexcept {
    ++g_metrics.target11_entries;
}

void record_target12_entry() noexcept {
    ++g_metrics.target12_entries;
}

void record_child_00f0d2() noexcept {
    ++g_metrics.child_calls_00f0d2;
}

void record_loopback() noexcept {
    ++g_metrics.loopback_joins;
}

void record_terminal_return() noexcept {
    ++g_metrics.terminal_returns;
}

void record_stepped_instructions(std::size_t count) noexcept {
    g_metrics.stepped_instructions += count;
}

void record_contract_fault() noexcept {
    ++g_metrics.contract_faults;
}

void reset_cluster_metrics() noexcept {
    g_metrics = {};
}

void write_cluster_metrics(const std::filesystem::path& path) {
    std::ofstream out(path, std::ios::trunc);
    if (!out) return;
    out << "{\n"
        << "  \"schema\": \"oasis.m68k.entity-cluster-metrics.v1\",\n"
        << "  \"cluster_entries\": " << g_metrics.cluster_entries << ",\n"
        << "  \"dispatcher_entries\": " << g_metrics.dispatcher_entries << ",\n"
        << "  \"state0_entries\": " << g_metrics.state0_entries << ",\n"
        << "  \"target10_entries\": " << g_metrics.target10_entries << ",\n"
        << "  \"target11_entries\": " << g_metrics.target11_entries << ",\n"
        << "  \"target12_entries\": " << g_metrics.target12_entries << ",\n"
        << "  \"child_calls_00f0d2\": " << g_metrics.child_calls_00f0d2 << ",\n"
        << "  \"loopback_joins\": " << g_metrics.loopback_joins << ",\n"
        << "  \"terminal_returns\": " << g_metrics.terminal_returns << ",\n"
        << "  \"stepped_instructions\": " << g_metrics.stepped_instructions << ",\n"
        << "  \"contract_faults\": " << g_metrics.contract_faults << "\n"
        << "}\n";
}

} // namespace oasis::hybrid::entity_cluster
