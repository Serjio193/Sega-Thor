#include "tools/hybrid/callee_62ae0_observer.hpp"

#include <stdexcept>

namespace {
unsigned reg(unsigned index) { return index == 13U ? 0x00FF001AU : 0U; }
int cycles() { return 100; }
void require(bool value) { if (!value) throw std::runtime_error("callee 0x62AE0 regression"); }
}

int main() {
    using oasis::hybrid::Callee62AE0Observer;
    const oasis::hybrid::CallerAttributionApi api{reg, nullptr, cycles, cycles};
    Callee62AE0Observer observer(api);
    auto execute = [&](unsigned pc) { observer.event(1, 0, pc, 0, 1, 0); };
    execute(0x0601E2U);
    execute(0x062AE0U);
    observer.event(2, 1, 0x00FF0013U, 0, 1, 0x062AE0U);
    execute(0x062AFEU);
    observer.event(2, 1, 0x00FF001AU, 1, 1, 0x062AFEU);
    execute(0x062B04U);
    execute(0x062B16U);
    observer.event(4, 1, 0x00FF077CU, 0xFFU, 1, 0x062B16U);
    execute(0x062B1AU);
    observer.event(2, 4, 0x00FF0B62U, 0, 1, 0x062B1AU);
    execute(0x0601E6U);
    observer.finish({});
    const auto& text = observer.jsonl();
    require(text.find("\"entries\":1,\"returns\":1") != std::string::npos);
    require(text.find("\"a5_equal\":1,\"a5_unequal\":0") != std::string::npos);
    require(text.find("\"direct_nested\":0,\"indirect_nested\":0") != std::string::npos);
    require(text.find("\"address\":16711699") != std::string::npos);
    require(text.find("\"address\":16711706") != std::string::npos);
    require(text.find("\"address\":16713596") != std::string::npos);
    return 0;
}
