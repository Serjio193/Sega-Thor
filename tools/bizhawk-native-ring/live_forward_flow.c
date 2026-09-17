#include "live_forward_trace.h"

static uint32_t condition_true(uint32_t cc, uint32_t ccr)
{
  uint32_t c = ccr & 1u;
  uint32_t v = (ccr >> 1) & 1u;
  uint32_t z = (ccr >> 2) & 1u;
  uint32_t n = (ccr >> 3) & 1u;
  switch (cc & 15u)
  {
    case 0: return 1;
    case 1: return 0;
    case 2: return !c && !z;
    case 3: return c || z;
    case 4: return !c;
    case 5: return c;
    case 6: return !z;
    case 7: return z;
    case 8: return !v;
    case 9: return v;
    case 10: return !n;
    case 11: return n;
    case 12: return n == v;
    case 13: return n != v;
    case 14: return !z && n == v;
    default: return z || n != v;
  }
}

uint32_t oasis_lf_flow_flags(uint16_t opcode, uint16_t ccr,
                             uint32_t dreg_value, uint32_t after_execution,
                             uint16_t *flags)
{
  uint32_t condition;
  if (!flags)
    return 0;
  if ((opcode & 0xf000u) == 0x6000u)
  {
    condition = (opcode & 0x0f00u) <= 0x0100u ? 1u :
      condition_true((opcode >> 8) & 15u, ccr);
    *flags |= condition ? OASIS_LF_BRANCH_TAKEN : OASIS_LF_BRANCH_NOT_TAKEN;
    return 1;
  }
  if ((opcode & 0xf0f8u) == 0x50c8u)
  {
    *flags |= OASIS_LF_CONTROL_FLOW;
    if (after_execution)
    {
      condition = condition_true((opcode >> 8) & 15u, ccr);
      condition = !condition && (dreg_value & 0xffffu) != 0xffffu;
      *flags |= condition ? OASIS_LF_BRANCH_TAKEN : OASIS_LF_BRANCH_NOT_TAKEN;
    }
    return 1;
  }
  if ((opcode & 0xffc0u) == 0x4ec0u ||
      (opcode & 0xffc0u) == 0x4e80u || opcode == 0x4e75u ||
      opcode == 0x4e73u || opcode == 0x4e77u)
  {
    *flags |= OASIS_LF_CONTROL_FLOW;
    return 1;
  }
  return 0;
}
