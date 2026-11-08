#pragma once

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"

#include <cstddef>
#include <cstdint>

namespace oasis::hybrid::entity_cluster {

inline constexpr unsigned kCommonJoinPc = 0x0255D8U;
inline constexpr unsigned kClusterDispatcherPc = 0x02575CU;
inline constexpr unsigned kJumpTableBasePc = 0x025768U;
inline constexpr unsigned kMaxValidTableIndex = 24U;
inline constexpr unsigned kTarget10EntryPc = 0x0257EEU;
inline constexpr unsigned kTarget10ExitPc = 0x02583CU;
inline constexpr unsigned kChild00F0D2Pc = 0x00F0D2U;
inline constexpr unsigned kTarget11EntryPc = 0x02583EU;
inline constexpr unsigned kTarget12EntryPc = 0x025880U;
inline constexpr unsigned kState0EntryPc = 0x025896U;
inline constexpr unsigned kTailJump00E7E8Pc = 0x00E7E8U;

enum class ClusterTarget : std::uint32_t {
    Unknown = 0,
    ModeEntry = 0x0254EA,
    CommonJoin = 0x0255D8,
    PreDispatcher = 0x0256B8,
    Dispatcher = 0x02575C,
    JumpTableBase = 0x025768,
    Target8 = 0x025782,
    Target9 = 0x0257D2,
    Target10 = 0x0257EE,
    Target11 = 0x02583E,
    Target12 = 0x025880,
    State0 = 0x025896,
    State1 = 0x0258A0,
    State2 = 0x025940,
    State3 = 0x025968,
    State4 = 0x0259AC,
    State5 = 0x0259CE,
    State6 = 0x025A72,
    State7 = 0x025B22,
};

enum class TransferKind : std::uint8_t {
    ContinueInsideCluster,
    LoopbackJoin,
    ExternalRomChild,
    TerminalReturn,
    ContractFault
};

struct ClusterTransfer {
    TransferKind kind{TransferKind::ContinueInsideCluster};
    unsigned next_pc{};
    int cycle_adjustment{};
};

struct ClusterMetrics {
    std::size_t cluster_entries{};
    std::size_t dispatcher_entries{};
    std::size_t state0_entries{};
    std::size_t target10_entries{};
    std::size_t target11_entries{};
    std::size_t target12_entries{};
    std::size_t child_calls_00f0d2{};
    std::size_t loopback_joins{};
    std::size_t terminal_returns{};
    std::size_t stepped_instructions{};
    std::size_t contract_faults{};
};

} // namespace oasis::hybrid::entity_cluster
