#pragma once

#include "tools/hybrid/caller_attribution.hpp"
#include <array>
#include <filesystem>
#include <string>

namespace oasis::hybrid {

class A5LifetimeObserver {
public:
    explicit A5LifetimeObserver(CallerAttributionApi api);
    void event(int type, int width, unsigned address, unsigned value,
               unsigned frame, unsigned previous_execute_pc);
    void finish(const std::filesystem::path& output);
    [[nodiscard]] const std::string& jsonl() const noexcept { return jsonl_; }
    [[nodiscard]] unsigned generations() const noexcept { return generations_; }
    [[nodiscard]] unsigned closed_generations() const noexcept { return closed_; }

private:
    CallerAttributionApi api_;
    std::string jsonl_;
    unsigned generations_{};
    unsigned closed_{};
    unsigned current_generation_{};
    unsigned base_a5_{};
    unsigned previous_pc_{};
    bool active_{};
    bool branch_pending_{};
    unsigned branch_pc_{};
    unsigned branch_target_{};
    unsigned branch_fallthrough_{};
    bool call_pending_{};
    unsigned call_site_{};
    unsigned call_target_{};
    unsigned call_return_{};
    bool write_pending_{};
    unsigned write_pc_{};
    unsigned write_address_{};
    unsigned write_old_{};
    std::array<unsigned, 16> memory_type_counts_{};

    [[nodiscard]] unsigned a5() const;
    [[nodiscard]] unsigned peek_byte(unsigned address) const;
    void append(const std::string& text);
    void execute(unsigned pc, unsigned frame);
    void memory(int type, int width, unsigned address, unsigned value, unsigned frame);
};

} // namespace oasis::hybrid
