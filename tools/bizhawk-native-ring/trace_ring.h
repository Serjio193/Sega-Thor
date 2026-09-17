#ifndef GPGX_TRACE_RING_H
#define GPGX_TRACE_RING_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GPGX_TRACE_RING_CAPACITY 4096u

typedef struct
{
  uint64_t sequence;
  uint32_t pc;
  uint16_t opcode;
  uint16_t reserved;
} gpgx_trace_record;

void gpgx_trace_ring_record(uint32_t pc, uint16_t opcode);
uint64_t gpgx_trace_ring_latest(void);
uint32_t gpgx_trace_ring_count(void);
uint32_t gpgx_trace_ring_copy(uint64_t start_sequence,
                              gpgx_trace_record *destination,
                              uint32_t capacity);
void gpgx_trace_ring_reset(void);

#ifdef __cplusplus
}
#endif

#endif
