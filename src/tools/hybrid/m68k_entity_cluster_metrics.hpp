#pragma once

#include "tools/hybrid/m68k_entity_cluster_types.hpp"

#include <filesystem>

namespace oasis::hybrid::entity_cluster {

[[nodiscard]] const ClusterMetrics& get_cluster_metrics() noexcept;
void record_cluster_entry() noexcept;
void record_dispatcher_entry() noexcept;
void record_state0_entry() noexcept;
void record_target10_entry() noexcept;
void record_target11_entry() noexcept;
void record_target12_entry() noexcept;
void record_child_00f0d2() noexcept;
void record_loopback() noexcept;
void record_terminal_return() noexcept;
void record_stepped_instructions(std::size_t count) noexcept;
void record_contract_fault() noexcept;
void reset_cluster_metrics() noexcept;
void write_cluster_metrics(const std::filesystem::path& path);

} // namespace oasis::hybrid::entity_cluster
