#include "tools/hybrid/callee_623ac_observer.hpp"

#include <stdexcept>

namespace {
unsigned reg(unsigned index) { return index == 13U ? 0x00FF001AU : 0U; }
int cycles() { return 100; }
void require(bool value) { if (!value) throw std::runtime_error("callee 0x623AC regression"); }
}

int main() {
    using oasis::hybrid::Callee623ACObserver;
    const oasis::hybrid::CallerAttributionApi api{reg, nullptr, cycles, cycles};
    Callee623ACObserver observer(api);
    auto execute = [&](unsigned pc) { observer.event(1, 0, pc, 0, 1, 0); };
    execute(0x060234U);
    execute(0x0623ACU);
    execute(0x0623B4U);
    observer.event(2, 1, 0x00FF001AU, 1, 1, 0x0623B4U);
    execute(0x0623CCU);
    execute(0x062732U);
    execute(0x0623D0U);
    observer.event(4, 1, 0x00C00011U, 0, 1, 0x0623D0U);
    execute(0x060238U);
    observer.finish({});
    const auto& text = observer.jsonl();
    require(text.find("\"entries\":1,\"returns\":1") != std::string::npos);
    require(text.find("\"a5_equal\":1,\"a5_unequal\":0") != std::string::npos);
    require(text.find("\"direct_calls\":1,\"indirect_calls\":0") != std::string::npos);
    require(text.find("\"direct_returns\":1,\"indirect_returns\":0") != std::string::npos);
    require(text.find("\"parent_calls\":[1,0,0,0]") != std::string::npos);
    require(text.find("\"paths\":[{\"hash\":") != std::string::npos);
    require(text.find("\"address\":16711706") != std::string::npos);
    require(text.find("\"address\":12582929") != std::string::npos);
    return 0;
}
