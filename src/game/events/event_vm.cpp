#include "game/events/event_vm.hpp"

#include <array>

namespace oasis::game::events {

namespace {

inline constexpr std::array<std::uint32_t, 31> kOpcodeTargets = {
    0x0000F91CU, 0x0000FCCEU, 0x0000F920U, 0x0000F942U, 0x0000F96AU, 0x0000FAACU,
    0x0000FC6CU, 0x0000FC94U, 0x0000FCA6U, 0x00010034U, 0x0000FCB8U, 0x0000FCC4U,
    0x00010086U, 0x000100AAU, 0x0000FD8CU, 0x00010140U, 0x00010170U, 0x000101A0U,
    0x0000FAB2U, 0x000101D0U, 0x000101E4U, 0x0000FEDEU, 0x0000FF1AU, 0x0000FE0AU,
    0x0000FF98U, 0x0000FFDEU, 0x000100D6U, 0x000100E4U, 0x000100EEU, 0x000100F8U,
    0x0000F854U,
};

std::uint16_t read_be16(std::span<const std::uint8_t> rom, std::size_t off) noexcept {
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(rom[off]) << 8U) |
                                      static_cast<std::uint16_t>(rom[off + 1U]));
}

std::int16_t read_be_s16(std::span<const std::uint8_t> rom, std::size_t off) noexcept {
    return static_cast<std::int16_t>(read_be16(rom, off));
}

std::uint32_t read_be32(std::span<const std::uint8_t> rom, std::size_t off) noexcept {
    return (static_cast<std::uint32_t>(rom[off]) << 24U) |
           (static_cast<std::uint32_t>(rom[off + 1U]) << 16U) |
           (static_cast<std::uint32_t>(rom[off + 2U]) << 8U) |
           static_cast<std::uint32_t>(rom[off + 3U]);
}

template <std::size_t N>
bool test_bit(const std::array<std::uint8_t, N>& arr, std::uint16_t bit_idx) noexcept {
    const std::size_t byte_idx = bit_idx >> 3U;
    if (byte_idx >= N) {
        return false;
    }
    return (arr[byte_idx] & static_cast<std::uint8_t>(1U << (bit_idx & 7U))) != 0U;
}

template <std::size_t N>
void write_bit(std::array<std::uint8_t, N>& arr, std::uint16_t bit_idx, bool set) noexcept {
    const std::size_t byte_idx = bit_idx >> 3U;
    if (byte_idx >= N) {
        return;
    }
    const auto mask = static_cast<std::uint8_t>(1U << (bit_idx & 7U));
    if (set) {
        arr[byte_idx] |= mask;
    } else {
        arr[byte_idx] &= static_cast<std::uint8_t>(~mask);
    }
}

std::uint16_t count_non_spirit_entities(const EventVmState& state) noexcept {
    std::uint16_t count = 0U;
    for (const auto& slot : state.entities) {
        if (slot.type_id > 0U && (slot.flags_3a & 1U) == 0U) {
            ++count;
        }
    }
    return count;
}

bool find_entity(const EventVmState& state, std::uint16_t type_id) noexcept {
    for (const auto& slot : state.entities) {
        if (slot.type_id == type_id) {
            return true;
        }
    }
    return false;
}

} // namespace

std::uint32_t lookup_event_vm_opcode_target(std::uint8_t opcode) noexcept {
    return opcode < kOpcodeTargets.size() ? kOpcodeTargets[opcode] : 0U;
}

bool evaluate_op12_comparator(std::uint8_t selector, std::uint16_t lhs, std::uint16_t rhs) noexcept {
    switch (selector) {
        case 0U: return lhs == rhs;
        case 2U: return lhs != rhs;
        case 4U: return lhs > rhs;
        case 6U: return lhs >= rhs;
        case 8U: return lhs <= rhs;
        case 10U: return lhs < rhs;
        default: return false;
    }
}

bool evaluate_op12_condition(
    const EventVmState& state,
    std::span<const std::uint8_t> rom,
    std::uint32_t instr_pc) noexcept {
    if (static_cast<std::size_t>(instr_pc) + 8U > rom.size()) {
        return false;
    }
    const std::uint8_t sub = rom[instr_pc + 2U];
    const std::uint8_t cmp_or_neg = rom[instr_pc + 3U];
    const std::uint16_t imm16 = read_be16(rom, instr_pc + 4U);

    switch (sub) {
        case 0x00U:
            return evaluate_op12_comparator(cmp_or_neg, count_non_spirit_entities(state), imm16);
        case 0x01U:
            return test_bit(state.global_flags, imm16) == (cmp_or_neg == 0U);
        case 0x02U:
            return test_bit(state.chest_flags, imm16) == (cmp_or_neg == 0U);
        case 0x03U:
            return test_bit(state.secondary_flags, imm16) == (cmp_or_neg == 0U);
        case 0x04U:
            return find_entity(state, imm16) == (cmp_or_neg == 0U);
        case 0x05U: {
            const std::uint16_t pct = (state.player_max_hp > 0U)
                ? static_cast<std::uint16_t>((static_cast<std::uint32_t>(state.player_hp) * 100U) / state.player_max_hp)
                : 0U;
            return evaluate_op12_comparator(cmp_or_neg, pct, imm16);
        }
        case 0x06U: {
            const std::uint16_t pct = (state.player_max_sp > 0U)
                ? static_cast<std::uint16_t>((static_cast<std::uint32_t>(state.player_sp) * 100U) / state.player_max_sp)
                : 0U;
            return evaluate_op12_comparator(cmp_or_neg, pct, imm16);
        }
        case 0x07U:
            return (state.active_spirit_id == imm16) == (cmp_or_neg == 0U);
        case 0x08U:
            return evaluate_op12_comparator(cmp_or_neg, state.player_x, imm16);
        case 0x09U:
            return evaluate_op12_comparator(cmp_or_neg, state.player_y, imm16);
        case 0x0AU:
            return (state.active_spirit_id == 0x001AU) &&
                   evaluate_op12_comparator(cmp_or_neg, state.spirit_x, imm16);
        case 0x0BU:
            return (state.active_spirit_id == 0x001AU) &&
                   evaluate_op12_comparator(cmp_or_neg, state.spirit_y, imm16);
        default:
            return false;
    }
}

void EventVmInterpreter::reset(std::uint32_t start_pc) noexcept {
    state_ = EventVmState{};
    state_.pc = start_pc;
}

bool EventVmInterpreter::execute_opcode(std::span<const std::uint8_t> rom) {
    if (state_.halted || state_.pc >= rom.size()) {
        return false;
    }
    const std::size_t pc = state_.pc;
    const std::uint8_t op = rom[pc];
    if (op >= kOpcodeTargets.size()) {
        state_.halted = true;
        return false;
    }

    switch (static_cast<EventVmOpcode>(op)) {
        case EventVmOpcode::EndScriptYield:
            state_.halted = true;
            return false;
        case EventVmOpcode::SetSpawnTable1: {
            state_.spawn_table_1.clear();
            std::size_t p = pc + 2U;
            while (p + 2U <= rom.size()) {
                const std::uint16_t v = read_be16(rom, p);
                p += 2U;
                if (v == 0xFFFFU) break;
                state_.spawn_table_1.push_back(v);
            }
            state_.pc = static_cast<std::uint32_t>(p);
            return true;
        }
        case EventVmOpcode::SetSpawnTable2: {
            state_.spawn_table_2.clear();
            std::size_t p = pc + 2U;
            while (p + 4U <= rom.size()) {
                const std::uint16_t a = read_be16(rom, p);
                if (a == 0xFFFFU) { p += 2U; break; }
                const std::uint16_t b = read_be16(rom, p + 2U);
                p += 4U;
                state_.spawn_table_2.emplace_back(a, b);
            }
            state_.pc = static_cast<std::uint32_t>(p);
            return true;
        }
        case EventVmOpcode::WaitUntilCondition:
            return false; // Polls external condition; yields without advancing PC
        case EventVmOpcode::JumpRel16:
            if (pc + 4U > rom.size()) return false;
            state_.pc = static_cast<std::uint32_t>(static_cast<std::int32_t>(pc) + read_be_s16(rom, pc + 2U));
            return true;
        case EventVmOpcode::LoopCounterDecBranch: {
            if (pc + 4U > rom.size()) return false;
            const std::size_t idx = rom[pc + 1U] & 7U;
            if (state_.loop_counters[idx] > 0U && --state_.loop_counters[idx] > 0U) {
                state_.pc = static_cast<std::uint32_t>(static_cast<std::int32_t>(pc) + read_be_s16(rom, pc + 2U));
            } else {
                state_.pc = static_cast<std::uint32_t>(pc + 4U);
            }
            return true;
        }
        case EventVmOpcode::LoopCounterSet:
            if (pc + 4U > rom.size()) return false;
            state_.loop_counters[rom[pc + 1U] & 7U] = read_be16(rom, pc + 2U);
            state_.pc = static_cast<std::uint32_t>(pc + 4U);
            return true;
        case EventVmOpcode::WaitFrames:
            if (pc + 2U > rom.size()) return false;
            state_.wait_frames = rom[pc + 1U];
            state_.pc = static_cast<std::uint32_t>(pc + 2U);
            return false;
        case EventVmOpcode::SetMaxActiveEnemies:
            if (pc + 2U > rom.size()) return false;
            state_.max_active_enemies = rom[pc + 1U];
            state_.pc = static_cast<std::uint32_t>(pc + 2U);
            return true;
        case EventVmOpcode::LoadPalette0C:
        case EventVmOpcode::LoadPalette0D:
            if (pc + 4U > rom.size()) return false;
            state_.loaded_palettes.push_back(read_be16(rom, pc + 2U));
            state_.pc = static_cast<std::uint32_t>(pc + 4U);
            return true;
        case EventVmOpcode::SetClearGlobalFlag:
        case EventVmOpcode::SetClearChestFlag:
        case EventVmOpcode::SetClearSecondaryFlag: {
            if (pc + 4U > rom.size()) return false;
            const bool set = rom[pc + 1U] != 0U;
            const std::uint16_t bit = read_be16(rom, pc + 2U);
            if (op == 0x0FU) write_bit(state_.global_flags, bit, set);
            else if (op == 0x10U) write_bit(state_.chest_flags, bit, set);
            else write_bit(state_.secondary_flags, bit, set);
            state_.pc = static_cast<std::uint32_t>(pc + 4U);
            return true;
        }
        case EventVmOpcode::BranchIfCondition:
            if (pc + 8U > rom.size()) return false;
            if (evaluate_op12_condition(state_, rom, static_cast<std::uint32_t>(pc))) {
                state_.pc = static_cast<std::uint32_t>(static_cast<std::int32_t>(pc) + read_be_s16(rom, pc + 6U));
            } else {
                state_.pc = static_cast<std::uint32_t>(pc + 8U);
            }
            return true;
        case EventVmOpcode::CallSubroutine24Bit:
            if (pc + 6U > rom.size()) return false;
            state_.return_pc = static_cast<std::uint32_t>(pc + 6U);
            state_.pc = read_be32(rom, pc + 2U) & 0x00FFFFFFU;
            return true;
        case EventVmOpcode::ReturnFromSubroutine:
            state_.pc = state_.return_pc;
            state_.return_pc = 0U;
            return state_.pc != 0U;
        case EventVmOpcode::BranchIfFlag15: {
            if (pc + 8U > rom.size()) return false;
            const std::uint8_t bank = rom[pc + 1U];
            const std::uint16_t bit = read_be16(rom, pc + 2U);
            const bool is_set = (bank == 0U) ? test_bit(state_.global_flags, bit)
                              : (bank == 1U) ? test_bit(state_.chest_flags, bit)
                                             : test_bit(state_.secondary_flags, bit);
            state_.pc = is_set
                ? static_cast<std::uint32_t>(static_cast<std::int32_t>(pc) + read_be_s16(rom, pc + 6U))
                : static_cast<std::uint32_t>(pc + 8U);
            return true;
        }
        case EventVmOpcode::PlaySoundCmd:
            if (pc + 6U > rom.size()) return false;
            state_.sound_commands.push_back(read_be32(rom, pc + 2U));
            state_.pc = static_cast<std::uint32_t>(pc + 6U);
            return true;
        case EventVmOpcode::SetEventVar1C:
            if (pc + 4U > rom.size()) return false;
            state_.event_var = read_be16(rom, pc + 2U);
            state_.pc = static_cast<std::uint32_t>(pc + 4U);
            return true;
        case EventVmOpcode::CameraPanOrFade1E:
            if (pc + 6U > rom.size()) return false;
            state_.pan_state = rom[pc + 1U];
            state_.player_lock_flag = 0xFFU;
            state_.pc = static_cast<std::uint32_t>(pc + 6U);
            return false;
        default:
            // Remaining spawn/entity/VDP/camera opcodes (0x01, 0x09, 0x0A, 0x0E, 0x16..0x1A, 0x1D)
            // advance past their nominal 4-byte descriptor when executed standalone.
            state_.pc = static_cast<std::uint32_t>(pc + 4U);
            return true;
    }
}

void EventVmInterpreter::step_frame(std::span<const std::uint8_t> rom, std::uint32_t max_ops_per_frame) {
    if ((state_.vm_flags & 0x02U) != 0U || state_.halted) {
        return;
    }
    if (state_.wait_frames > 0U) {
        --state_.wait_frames;
        if (state_.wait_frames > 0U) {
            return;
        }
    }
    if (state_.pan_state != 0U) {
        return;
    }
    for (std::uint32_t i = 0U; i < max_ops_per_frame; ++i) {
        if (!execute_opcode(rom)) {
            break;
        }
    }
}

} // namespace oasis::game::events
