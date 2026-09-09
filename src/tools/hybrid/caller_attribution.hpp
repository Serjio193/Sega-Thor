#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace oasis::hybrid {

struct CallerAttributionApi {
    unsigned (*reg)(unsigned){};
    int (*peek)(unsigned){};
    int (*cycles)(){};
    int (*refresh_cycles)(){};
};

enum class CallerClassification {
    Caller604F6,
    Caller60BCC,
    OtherProvenCaller,
    UnknownCaller,
};

struct CallerAttributionRecord {
    unsigned invocation_ordinal{};
    CallerClassification classification{CallerClassification::UnknownCaller};
    std::uint32_t caller_pc{};
    std::uint32_t call_site_pc{};
    std::uint32_t return_pc{};
    std::uint32_t stack_a7{};
    unsigned entry_frame{};
    int entry_cycles{};
    int entry_refresh_cycles{};
    std::array<unsigned, 18> registers{};
    std::string entry_source;
};

class CallerAttributionObserver {
public:
    CallerAttributionObserver(std::filesystem::path output, CallerAttributionApi api);
    void entry(std::uint32_t target, std::uint32_t previous_execute_pc, unsigned frame);
    void finish();
    [[nodiscard]] const std::vector<CallerAttributionRecord>& records() const noexcept {
        return records_;
    }
    [[nodiscard]] unsigned unknown_count() const noexcept { return unknown_count_; }

private:
    std::filesystem::path output_;
    CallerAttributionApi api_;
    std::vector<CallerAttributionRecord> records_;
    unsigned unknown_count_{};
    bool finished_{};
};

[[nodiscard]] const char* caller_classification_name(CallerClassification value) noexcept;
[[nodiscard]] std::string caller_attribution_to_json(
    const std::vector<CallerAttributionRecord>& records, unsigned unknown_count);

} // namespace oasis::hybrid
