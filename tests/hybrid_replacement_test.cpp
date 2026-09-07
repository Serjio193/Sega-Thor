#include "tools/hybrid/replacement.hpp"

#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
using oasis::hybrid::Replacement;
using oasis::hybrid::ReplacementMetrics;
class Fake final : public Replacement {
public:
    explicit Fake(unsigned address) : address_(address) {}
    void hook(int type, int, unsigned address, unsigned) noexcept override {
        if (type == 1 && address == address_) {
            ++metrics_.calls;
            ++metrics_.override_calls;
        }
    }
    unsigned target_address() const override { return address_; }
    bool complete() const override { return !active_; }
    const std::string& error() const override { return error_; }
    ReplacementMetrics metrics() const override { return metrics_; }
private:
    unsigned address_;
    bool active_{};
    std::string error_;
    ReplacementMetrics metrics_{};
};
void check(bool value) { if (!value) throw std::runtime_error("replacement registry regression"); }
}

int main() {
    try {
        Fake first(0x100), second(0x200);
        oasis::hybrid::Registry registry({&first, &second});
        registry.hook(1, 0, 0x100, 0);
        check(first.metrics().calls == 1 && second.metrics().calls == 0);
        registry.hook(1, 0, 0x123, 0);
        registry.hook(1, 0, 0x200, 0);
        check(registry.totals().override_calls == 2);
        check(registry.totals().body_instructions == 0);
        check(registry.complete() && registry.error().empty());
        std::cout << "developer-only replacement registry routing verified\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
