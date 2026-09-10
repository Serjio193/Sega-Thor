#pragma once

#include "tools/hybrid/caller_attribution.hpp"
#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace oasis::hybrid {

class Callee623ACObserver {
public:
    explicit Callee623ACObserver(CallerAttributionApi api);
    void event(int type, int width, unsigned address, unsigned value,
               unsigned frame, unsigned previous_execute_pc);
    void finish(const std::filesystem::path& output);
    [[nodiscard]] const std::string& jsonl() const noexcept { return jsonl_; }

private:
    struct Effect {
        unsigned pc{}, address{}, type{}, width{}, count{}, first_order{}, last_order{};
        std::vector<unsigned> path_hashes;
    };
    struct Path { unsigned hash{}, length{}, count{}; std::vector<unsigned> pcs; };
    struct Nested { unsigned callsite{}, target{}, return_pc{}, calls{}, returns{}, a5_equal{}, a5_unequal{}; };
    struct Pending { unsigned callsite{}, target{}, return_pc{}, entry_a5{}; bool indirect{}; };
    CallerAttributionApi api_;
    std::string jsonl_;
    unsigned entries_{}, returns_{}, equal_a5_{}, unequal_a5_{};
    unsigned direct_calls_{}, indirect_calls_{}, direct_returns_{}, indirect_returns_{};
    unsigned interrupt_events_{};
    bool parent_pending_{}, active_{}, direct_pending_{}, indirect_pending_{};
    unsigned parent_return_{};
    unsigned entry_a5_{}, path_hash_{2166136261U}, path_length_{};
    std::vector<unsigned> path_pcs_;
    std::vector<Effect> effects_;
    std::vector<std::size_t> current_effects_;
    std::vector<Path> paths_;
    std::vector<Nested> nested_;
    Pending pending_{};
    std::vector<Pending> stack_;
    std::array<unsigned, 16> hook_types_{};
    std::array<unsigned, 4> parent_calls_{};
    std::array<unsigned, 4> parent_returns_{};

    [[nodiscard]] unsigned a5() const;
    [[nodiscard]] static bool parent_call(unsigned pc, unsigned& return_pc);
    [[nodiscard]] static bool direct_call(unsigned pc, unsigned& target);
    [[nodiscard]] static bool indirect_call(unsigned pc);
    [[nodiscard]] Nested& nested(unsigned callsite, unsigned target, unsigned return_pc);
    void execute(unsigned pc);
    void memory(int type, int width, unsigned address, unsigned previous_pc);
    void finish_nested(unsigned pc);
    void finish_parent();
};

} // namespace oasis::hybrid
