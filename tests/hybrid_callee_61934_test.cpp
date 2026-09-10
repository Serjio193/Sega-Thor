#include "tools/hybrid/callee_61934_observer.hpp"

#include <stdexcept>

namespace {
unsigned reg(unsigned index) { return index == 13U ? 0x00FF001AU : 0U; }
int cycles() { return 100; }
void require(bool value) { if (!value) throw std::runtime_error("callee 0x61934 regression"); }
}

int main() {
    using oasis::hybrid::Callee61934Observer;
    const oasis::hybrid::CallerAttributionApi api{reg, nullptr, cycles, cycles};
    Callee61934Observer observer(api);
    auto execute = [&](unsigned pc) { observer.event(1, 0, pc, 0, 1, 0); };
    execute(0x0601EEU);
    execute(0x061934U);
    execute(0x06193CU);
    observer.event(2, 1, 0x00FF001AU, 1, 1, 0x06193CU);
    execute(0x061942U);
    execute(0x061946U);
    observer.event(4, 1, 0x00FF001EU, 7, 1, 0x061946U);
    execute(0x061958U);
    execute(0x0601F2U);
    observer.finish({});
    const auto& text = observer.jsonl();
    require(text.find("\"entries\":1,\"returns\":1") != std::string::npos);
    require(text.find("\"a5_equal\":1,\"a5_unequal\":0") != std::string::npos);
    require(text.find("\"direct_calls\":0,\"indirect_calls\":0") != std::string::npos);
    require(text.find("\"address\":16711706") != std::string::npos);
    require(text.find("\"address\":16711710") != std::string::npos);
    return 0;
}
