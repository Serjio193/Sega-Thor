#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace oasis::hybrid {

struct CandidateApi {
    unsigned (*reg)(unsigned);
    void (*set_reg)(unsigned, unsigned);
    int (*peek)(unsigned);
    void (*poke)(unsigned, int, unsigned);
    void (*set_return_state)(unsigned, unsigned, unsigned){};
    int (*cycles)(){};
    void (*add_cycles)(int){};
    int (*refresh_cycles)(){};
};

struct ReplacementMetrics {
    unsigned calls{};
    unsigned comparisons{};
    unsigned divergences{};
    unsigned body_instructions{};
    unsigned override_calls{};
    unsigned interrupts{};
};

class Replacement {
public:
    virtual ~Replacement() = default;
    virtual unsigned target_address() const = 0;
    virtual void hook(int type, int width, unsigned address, unsigned value) noexcept = 0;
    virtual bool complete() const = 0;
    virtual const std::string& error() const = 0;
    virtual ReplacementMetrics metrics() const = 0;
};

class Registry {
public:
    explicit Registry(std::vector<Replacement*> targets) : targets_(std::move(targets)) {}
    void hook(int type, int width, unsigned address, unsigned value) noexcept;
    bool complete() const;
    const std::string& error() const { return error_; }
    ReplacementMetrics totals() const;
    const std::vector<Replacement*>& targets() const { return targets_; }

private:
    std::vector<Replacement*> targets_;
    Replacement* active_{};
    std::string error_;
};

} // namespace oasis::hybrid
