#include "tools/hybrid/a5_lifetime_observer.hpp"

#include <array>
#include <stdexcept>

namespace {
unsigned current_a5 = 0x00FF001AU;
std::array<unsigned, 0x30> memory{};
unsigned reg(unsigned index) { return index == 13U ? current_a5 : 0U; }
int peek(unsigned address) {
    if (address < 0x00FF001AU || address >= 0x00FF004AU) return 0;
    return static_cast<int>(memory[address - 0x00FF001AU]);
}
int cycles() { return 900; }
void require(bool value) { if (!value) throw std::runtime_error("A5 lifetime regression"); }
}

int main() {
    using oasis::hybrid::A5LifetimeObserver;
    const oasis::hybrid::CallerAttributionApi api{reg, peek, cycles, cycles};
    A5LifetimeObserver observer(api);
    auto execute = [&](unsigned pc) { observer.event(1, 0, pc, 0, 7, 0); };
    execute(0x060182U);
    execute(0x060188U);
    execute(0x06018EU);
    execute(0x0601D4U);
    execute(0x06193CU);
    observer.event(2, 1, 0x00FF001AU, 1, 7, 0x06193CU);
    execute(0x061946U);
    observer.event(4, 1, 0x00FF001EU, 7, 7, 0x061946U);
    execute(0x0601EEU);
    execute(0x061934U);
    execute(0x0601F2U);
    execute(0x06027EU);
    current_a5 = 0x00C00000U;
    execute(0x060282U);
    observer.finish({});
    const auto& text = observer.jsonl();
    require(observer.generations() == 1U && observer.closed_generations() == 1U);
    require(text.find("\"event\":\"branch\",\"outcome\":\"taken\"") != std::string::npos);
    require(text.find("\"pc\":399676") != std::string::npos); // 0x6193C
    require(text.find("\"effective_address\":16711706") != std::string::npos);
    require(text.find("\"offset\":4,\"width\":1,\"direction\":\"write\"") != std::string::npos);
    require(text.find("\"event\":\"call_entry\",\"target\":399668") != std::string::npos);
    require(text.find("\"event\":\"call_return\"") != std::string::npos);
    require(text.find("\"event\":\"kill\"") != std::string::npos);
    return 0;
}
