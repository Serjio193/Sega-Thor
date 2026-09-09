#include "tools/hybrid/mechanical_primitive.hpp"

#include <cassert>
#include <sstream>

int main() {
    using namespace oasis::hybrid;
    std::ostringstream log;
    MechanicalPrimitiveRegistry registry({}, BasicBlockMode::NATIVE_OVERRIDE, log);
    const auto contracts = registry.contracts();
    assert(contracts.size() == 4U);
    assert(registry.handles(0x003A0CU) && registry.handles(0x0038A0U));
    assert(contracts[0].source_register == 2U && contracts[0].destination_register == 1U);
    assert(contracts[1].counter_register == 0U && contracts[2].width == 2U);
    assert(contracts[3].body_opcode == 0x421DU && contracts[3].continuation_pc == 0x06126CU);
    const auto portable = contracts[0].portable();
    assert(portable.body_token == contracts[0].body_pc && portable.width == 1U);
    return 0;
}
