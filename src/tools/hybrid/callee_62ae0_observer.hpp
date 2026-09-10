#pragma once

#include "tools/hybrid/caller_attribution.hpp"
#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace oasis::hybrid {

class Callee62AE0Observer {
public:
    explicit Callee62AE0Observer(CallerAttributionApi api);
    void event(int type, int width, unsigned address, unsigned value,
               unsigned frame, unsigned previous_execute_pc);
    void finish(const std::filesystem::path& output);
    [[nodiscard]] const std::string& jsonl() const noexcept { return jsonl_; }

private:
    CallerAttributionApi api_;
    std::string jsonl_;
    unsigned entries_{};
    unsigned returns_{};
    unsigned equal_a5_{};
    unsigned unequal_a5_{};
    unsigned direct_nested_{};
    unsigned indirect_nested_{};
    bool call_pending_{};
    bool active_{};
    unsigned entry_a5_{};
    unsigned path_hash_{2166136261U};
    unsigned path_length_{};
    std::vector<unsigned> path_pcs_;
    struct Effect {
        unsigned pc{};
        unsigned address{};
        unsigned type{};
        unsigned width{};
        unsigned count{};
    };
    struct Path { unsigned hash{}; unsigned length{}; unsigned count{}; std::vector<unsigned> pcs; };
    std::vector<Effect> effects_;
    std::vector<Path> paths_;
    std::array<unsigned, 16> hook_type_counts_{};

    [[nodiscard]] unsigned a5() const;
    void execute(unsigned pc);
    void memory(int type, int width, unsigned address, unsigned frame,
                unsigned previous_execute_pc);
    void finish_call();
};

} // namespace oasis::hybrid
