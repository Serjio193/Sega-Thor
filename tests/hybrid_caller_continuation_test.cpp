#include "tools/hybrid/caller_continuation.hpp"
#include <stdexcept>

namespace {
unsigned reg(unsigned index) { return index == 15 ? 0xFF0BB0U : index; }
int peek(unsigned address) { return static_cast<int>(address & 255U); }
int cycles() { return 123; }
void require(bool value) { if (!value) throw std::runtime_error("continuation evidence regression"); }
}

int main() {
    using oasis::hybrid::CallerContinuationObserver;
    const oasis::hybrid::CallerAttributionApi api{reg, peek, cycles, cycles};
    CallerContinuationObserver observer(api);
    auto execute = [&](unsigned pc) { observer.event(1, 0, pc, 0, 115, 0x604EA); };
    execute(0x611DE); // A different enclosing return cannot close this path.
    require(!observer.complete());
    execute(0x60004);
    execute(0x604F0);
    execute(0x604BC);
    execute(0x604E4); // Nested return is not the enclosing exit.
    execute(0x604FA);
    require(!observer.complete());
    execute(0x611DE);
    observer.event(16384, 0, 0x2234, 0, 115, 0x611DE);
    require(!observer.complete());
    execute(0x2234);
    require(observer.complete() && observer.entries() == 1);
    const auto text = observer.jsonl();
    execute(0x60BCC); // Unrelated hardware path must not enter the evidence.
    require(text == observer.jsonl());
    require(text.find("\"address\":8756") != std::string::npos);
    CallerContinuationObserver bounded(api);
    bounded.event(1, 0, 0x60004, 0, 1, 0);
    bounded.event(1, 0, 0x604F0, 0, 1, 0);
    for (unsigned i = 0; i < 513; ++i) bounded.event(1, 0, 0x604FA, 0, 1, 0);
    require(!bounded.complete());
    require(bounded.jsonl().find("\"truncated\":true") != std::string::npos);
    CallerContinuationObserver unowned(api);
    unowned.event(1, 0, 0x604F0, 0, 1, 0);
    unowned.event(1, 0, 0x611DE, 0, 1, 0);
    unowned.event(1, 0, 0x2234, 0, 1, 0);
    require(unowned.entries() == 1 && !unowned.complete());
}
