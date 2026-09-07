#pragma once
#include "tools/hybrid/contract.hpp"
#include <ostream>

namespace oasis::hybrid {
struct Api {
    unsigned (*reg)(unsigned);
    int (*peek)(unsigned);
};
class Dispatch {
public:
    Dispatch(Api api, Mode mode, std::span<const std::uint8_t> rom, std::ostream& log);
    void hook(int type, int width, unsigned address, unsigned value) noexcept;
    void frame(unsigned value) { frame_ = value; }
    bool complete() const { return !active_ && error_.empty(); }
    const std::string& error() const { return error_; }
    unsigned calls{}, comparisons{}, divergences{}, body_instructions{}, interrupts{};
private:
    State state() const;
    std::vector<std::uint8_t> bytes(unsigned address, std::size_t count) const;
    void begin();
    void finish();
    void event(int type, int width, unsigned address, unsigned value);
    Api api_;
    Mode mode_;
    std::span<const std::uint8_t> rom_;
    std::ostream& log_;
    bool active_{}, in_interrupt_{};
    unsigned frame_{}, entry_frame_{}, return_pc_{}, stack_base_{}, current_pc_{};
    State entry_{}, suspended_{};
    std::vector<std::uint8_t> source_, initial_output_, initial_stack_, output_writes_;
    std::vector<Write> stack_writes_;
    std::string error_;
    unsigned instructions_{};
};
}
