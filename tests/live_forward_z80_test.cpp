/* W3 Z80 co-capture + cross-CPU timeline V1 test suite.
 *
 * Tests A-V per §18 specification covering:
 * - Z80 instruction capture with exact byte forms
 * - Z80 bus events with domain classification
 * - Bank register change and resolved physical addresses
 * - YM2612/PSG writes
 * - Cross-CPU master_time ordering
 * - M68K regression
 * - Frame identity
 * - Determinism */

#include "live_forward_trace.h"

#include <cassert>
#include <cstring>
#include <utility>

static oasis_lf_cpu_state cpu_state(uint32_t pc)
{
  oasis_lf_cpu_state s{};
  s.pc = pc;
  return s;
}

static void m68k_execute(uint16_t opcode, uint32_t pc, uint32_t next_pc)
{
  oasis_lf_cpu_state before = cpu_state(pc);
  oasis_lf_cpu_state after = cpu_state(next_pc);
  oasis_lf_instruction_begin(pc, &before);
  oasis_lf_instruction_set_opcode(opcode, &before);
  oasis_lf_instruction_end(next_pc, &after, 0);
}

/* Test A: Z80 PC / next_PC exact. */
static void test_z80_pc_exact()
{
  assert(oasis_lf_configure(1, 20, 64u * 1024u));
  uint8_t nop[] = {0x00};
  oasis_lf_z80_instruction(0x0040, 0x0041, nop, 1);
  oasis_lf_record r{};
  assert(oasis_lf_ring_record(1, &r));
  assert(r.pc == 0x0040u);
  assert(r.address == 0x0041u);
  assert(r.cpu_id == OASIS_LF_CPU_Z80);
  assert((r.kind_flags & OASIS_LF_INSTRUCTION) != 0);
  assert((r.kind_flags & OASIS_LF_COMPLETE) != 0);
}

/* Test B: Normal Z80 opcode bytes. */
static void test_z80_normal_opcode()
{
  assert(oasis_lf_configure(1, 20, 64u * 1024u));
  uint8_t ld_a_n[] = {0x3E, 0x42};
  oasis_lf_z80_instruction(0x0100, 0x0102, ld_a_n, 2);
  oasis_lf_record r{};
  assert(oasis_lf_ring_record(1, &r));
  assert(r.value == 0x0000423Eu);
  assert(r.length_or_width == 2);
}

/* Test C: CB-prefixed exact. */
static void test_z80_cb_prefix()
{
  assert(oasis_lf_configure(1, 20, 64u * 1024u));
  uint8_t bit_7_a[] = {0xCB, 0x7F};
  oasis_lf_z80_instruction(0x0200, 0x0202, bit_7_a, 2);
  oasis_lf_record r{};
  assert(oasis_lf_ring_record(1, &r));
  assert(r.value == 0x00007FCBu);
  assert(r.length_or_width == 2);
}

/* Test D: ED-prefixed exact. */
static void test_z80_ed_prefix()
{
  assert(oasis_lf_configure(1, 20, 64u * 1024u));
  uint8_t ldir[] = {0xED, 0xB0};
  oasis_lf_z80_instruction(0x0300, 0x0302, ldir, 2);
  oasis_lf_record r{};
  assert(oasis_lf_ring_record(1, &r));
  assert(r.value == 0x0000B0EDu);
  assert(r.length_or_width == 2);
}

/* Test E: DD/FD instruction exact. */
static void test_z80_dd_fd_prefix()
{
  assert(oasis_lf_configure(1, 20, 64u * 1024u));
  uint8_t ld_ix_nn[] = {0xDD, 0x21, 0x34, 0x12};
  oasis_lf_z80_instruction(0x0400, 0x0404, ld_ix_nn, 4);
  oasis_lf_record r{};
  assert(oasis_lf_ring_record(1, &r));
  assert(r.value == 0x123421DDu);
  assert(r.length_or_width == 4);
}

/* Test F: DD-CB/FD-CB form exact (4 bytes). */
static void test_z80_ddcb_fdcb_prefix()
{
  assert(oasis_lf_configure(1, 20, 64u * 1024u));
  uint8_t bit_0_ix_d[] = {0xDD, 0xCB, 0x05, 0x46};
  oasis_lf_z80_instruction(0x0500, 0x0504, bit_0_ix_d, 4);
  oasis_lf_record r{};
  assert(oasis_lf_ring_record(1, &r));
  assert(r.value == 0x4605CBDDu);
  assert(r.length_or_width == 4);
  assert(r.pc == 0x0500u && r.address == 0x0504u);
}

/* Test G: Z80 opcode fetch excluded from DATA_READ. */
static void test_z80_fetch_not_data_read()
{
  assert(oasis_lf_configure(1, 20, 64u * 1024u));
  uint8_t nop[] = {0x00};
  oasis_lf_z80_instruction(0x0600, 0x0601, nop, 1);
  assert(oasis_lf_stream_sequence() == 1);
  oasis_lf_record r{};
  assert(oasis_lf_ring_record(1, &r));
  assert((r.kind_flags & OASIS_LF_INSTRUCTION) != 0);
  assert((r.kind_flags & OASIS_LF_EVENT) == 0);
}

/* Test H: Z80 RAM read exact. */
static void test_z80_ram_read()
{
  assert(oasis_lf_configure(1, 20, 64u * 1024u));
  uint8_t nop[] = {0x00};
  oasis_lf_z80_instruction(0x0700, 0x0701, nop, 1);
  oasis_lf_z80_bus_read(0x0700, 0x1800, 0xABu, 8,
                        OASIS_LF_BUS_Z80_RAM, 0x1800);
  oasis_lf_record r{};
  assert(oasis_lf_ring_record(2, &r));
  assert(r.cpu_id == OASIS_LF_CPU_Z80);
  assert(r.address == 0x1800u);
  assert(r.value == 0xABu);
  assert(r.length_or_width == 8);
  assert(r.domain == OASIS_LF_BUS_Z80_RAM);
  assert(((r.kind_flags & OASIS_LF_EVENT_SUBTYPE_MASK) >>
          OASIS_LF_EVENT_SUBTYPE_SHIFT) == OASIS_LF_EVENT_BUS_READ);
}

/* Test I: Z80 RAM write exact. */
static void test_z80_ram_write()
{
  assert(oasis_lf_configure(1, 20, 64u * 1024u));
  uint8_t nop[] = {0x00};
  oasis_lf_z80_instruction(0x0800, 0x0801, nop, 1);
  oasis_lf_z80_bus_write(0x0800, 0x1900, 0xCDu, 8,
                         OASIS_LF_BUS_Z80_RAM, 0x1900);
  oasis_lf_record r{};
  assert(oasis_lf_ring_record(2, &r));
  assert(r.cpu_id == OASIS_LF_CPU_Z80);
  assert(r.value == 0xCDu);
  assert(r.domain == OASIS_LF_BUS_Z80_RAM);
  assert(((r.kind_flags & OASIS_LF_EVENT_SUBTYPE_MASK) >>
          OASIS_LF_EVENT_SUBTYPE_SHIFT) == OASIS_LF_EVENT_BUS_WRITE);
}

/* Test J: Bank register change exact. */
static void test_bank_register_change()
{
  assert(oasis_lf_configure(1, 20, 64u * 1024u));
  oasis_lf_z80_bank_register_change(0x0900, 0x12u, 0x090000u);
  oasis_lf_record r{};
  assert(oasis_lf_ring_record(1, &r));
  assert(r.cpu_id == OASIS_LF_CPU_Z80);
  assert(r.value == 0x12u);
  assert(r.auxiliary == 0x090000u);
  assert(((r.kind_flags & OASIS_LF_EVENT_SUBTYPE_MASK) >>
          OASIS_LF_EVENT_SUBTYPE_SHIFT) ==
         OASIS_LF_EVENT_BANK_REGISTER_CHANGE);
}

/* Test K: Banked ROM read with resolved physical address. */
static void test_banked_rom_physical_address()
{
  assert(oasis_lf_configure(1, 20, 64u * 1024u));
  uint8_t nop[] = {0x00};
  oasis_lf_z80_instruction(0x0A00, 0x0A01, nop, 1);
  oasis_lf_z80_bus_read(0x0A00, 0x8010, 0x55u, 8,
                        OASIS_LF_BUS_BANKED_ROM, 0x090010u);
  oasis_lf_record r{};
  assert(oasis_lf_ring_record(2, &r));
  assert(r.address == 0x8010u);
  assert(r.auxiliary == 0x090010u);
  assert(r.value == 0x55u);
  assert(r.domain == OASIS_LF_BUS_BANKED_ROM);
}

/* Test L: YM2612 address write. */
static void test_ym2612_address_write()
{
  assert(oasis_lf_configure(1, 20, 64u * 1024u));
  uint8_t nop[] = {0x00};
  oasis_lf_z80_instruction(0x0B00, 0x0B01, nop, 1);
  oasis_lf_z80_bus_write(0x0B00, 0x4000, 0x28u, 8,
                         OASIS_LF_BUS_YM2612, 0);
  oasis_lf_record r{};
  assert(oasis_lf_ring_record(2, &r));
  assert(r.domain == OASIS_LF_BUS_YM2612);
  assert(r.address == 0x4000u);
  assert(r.value == 0x28u);
}

/* Test M: YM2612 data write. */
static void test_ym2612_data_write()
{
  assert(oasis_lf_configure(1, 20, 64u * 1024u));
  uint8_t nop[] = {0x00};
  oasis_lf_z80_instruction(0x0C00, 0x0C01, nop, 1);
  oasis_lf_z80_bus_write(0x0C00, 0x4001, 0xF0u, 8,
                         OASIS_LF_BUS_YM2612, 0);
  oasis_lf_record r{};
  assert(oasis_lf_ring_record(2, &r));
  assert(r.domain == OASIS_LF_BUS_YM2612);
  assert(r.address == 0x4001u);
  assert(r.value == 0xF0u);
}

/* Test N: Ordered YM2612 register write (addr then data). */
static void test_ym2612_ordered_register_write()
{
  assert(oasis_lf_configure(1, 20, 64u * 1024u));
  uint8_t nop[] = {0x00};
  oasis_lf_z80_instruction(0x0D00, 0x0D01, nop, 1);
  oasis_lf_z80_bus_write(0x0D00, 0x4000, 0x28u, 8,
                         OASIS_LF_BUS_YM2612, 0);
  oasis_lf_z80_bus_write(0x0D00, 0x4001, 0xF0u, 8,
                         OASIS_LF_BUS_YM2612, 0);
  oasis_lf_record r0{}, r1{};
  assert(oasis_lf_ring_record(2, &r0));
  assert(oasis_lf_ring_record(3, &r1));
  assert(r0.stream_sequence < r1.stream_sequence);
  assert(r0.address == 0x4000u && r0.value == 0x28u);
  assert(r1.address == 0x4001u && r1.value == 0xF0u);
}

/* Test O: PSG write exact. */
static void test_psg_write()
{
  assert(oasis_lf_configure(1, 20, 64u * 1024u));
  uint8_t nop[] = {0x00};
  oasis_lf_z80_instruction(0x0E00, 0x0E01, nop, 1);
  oasis_lf_z80_bus_write(0x0E00, 0x7F11, 0x9Fu, 8,
                         OASIS_LF_BUS_PSG, 0);
  oasis_lf_record r{};
  assert(oasis_lf_ring_record(2, &r));
  assert(r.domain == OASIS_LF_BUS_PSG);
  assert(r.value == 0x9Fu);
}

/* Test P: DAC register visible as ordinary YM2612 traffic. */
static void test_dac_as_ym2612()
{
  assert(oasis_lf_configure(1, 20, 64u * 1024u));
  uint8_t nop[] = {0x00};
  oasis_lf_z80_instruction(0x0F00, 0x0F01, nop, 1);
  oasis_lf_z80_bus_write(0x0F00, 0x4000, 0x2Au, 8,
                         OASIS_LF_BUS_YM2612, 0);
  oasis_lf_z80_bus_write(0x0F00, 0x4001, 0x80u, 8,
                         OASIS_LF_BUS_YM2612, 0);
  oasis_lf_record r{};
  assert(oasis_lf_ring_record(3, &r));
  assert(r.domain == OASIS_LF_BUS_YM2612);
  assert(r.value == 0x80u);
}

/* Test Q: M68K instruction_sequence unchanged. */
static void test_m68k_unchanged()
{
  assert(oasis_lf_configure(1, 20, 64u * 1024u));
  m68k_execute(0x4e71u, 0x100u, 0x102u);
  m68k_execute(0x4e71u, 0x102u, 0x104u);
  oasis_lf_record r0{}, r1{};
  assert(oasis_lf_ring_record(1, &r0));
  assert(oasis_lf_ring_record(2, &r1));
  assert(r0.cpu_id == OASIS_LF_CPU_68K);
  assert(r0.instruction_sequence == 1);
  assert(r1.instruction_sequence == 2);
  assert(r0.value == 0x4e71u);
}

/* Test R: Z80 records filtered by cpu_id. */
static void test_z80_filter()
{
  assert(oasis_lf_configure(1, 20, 64u * 1024u));
  m68k_execute(0x4e71u, 0x200u, 0x202u);
  uint8_t nop[] = {0x00};
  oasis_lf_z80_instruction(0x0040, 0x0041, nop, 1);
  m68k_execute(0x4e71u, 0x202u, 0x204u);
  uint32_t m68k_count = 0;
  uint32_t z80_count = 0;
  for (uint64_t seq = 1; seq <= oasis_lf_stream_sequence(); ++seq)
  {
    oasis_lf_record r{};
    assert(oasis_lf_ring_record(seq, &r));
    if ((r.kind_flags & OASIS_LF_INSTRUCTION) &&
        r.cpu_id == OASIS_LF_CPU_68K)
      m68k_count++;
    if ((r.kind_flags & OASIS_LF_INSTRUCTION) &&
        r.cpu_id == OASIS_LF_CPU_Z80)
      z80_count++;
  }
  assert(m68k_count == 2);
  assert(z80_count == 1);
}

/* Test S: Frame identity unchanged. */
static void test_frame_identity()
{
  assert(oasis_lf_configure(1, 20, 64u * 1024u));
  oasis_lf_frame_boundary();
  oasis_lf_frame_boundary();
  oasis_lf_record r0{}, r1{};
  assert(oasis_lf_ring_record(1, &r0));
  assert(oasis_lf_ring_record(2, &r1));
  assert(r0.pc == 1u && r1.pc == 2u);
  assert(r0.cpu_id == OASIS_LF_CPU_NONE);
}

/* Test T: Cross-CPU master_time ordering. */
static void test_cross_cpu_master_time()
{
  assert(oasis_lf_configure(1, 20, 64u * 1024u));
  oasis_lf_set_master_time(1000);
  m68k_execute(0x4e71u, 0x300u, 0x302u);
  oasis_lf_set_master_time(800);
  uint8_t nop[] = {0x00};
  oasis_lf_z80_instruction(0x0040, 0x0041, nop, 1);
  oasis_lf_set_master_time(1500);
  m68k_execute(0x4e71u, 0x302u, 0x304u);
  oasis_lf_record r0{}, r1{}, r2{};
  assert(oasis_lf_ring_record(1, &r0));
  assert(oasis_lf_ring_record(2, &r1));
  assert(oasis_lf_ring_record(3, &r2));
  assert(r0.master_time == 1000);
  assert(r1.master_time == 800);
  assert(r2.master_time == 1500);
  assert(r0.cpu_id == OASIS_LF_CPU_68K);
  assert(r1.cpu_id == OASIS_LF_CPU_Z80);
  assert(r2.cpu_id == OASIS_LF_CPU_68K);
  assert(r1.master_time < r0.master_time);
  assert(r0.master_time < r2.master_time);
}

/* Test U: No false causal relation. */
static void test_no_false_causal()
{
  assert(oasis_lf_configure(1, 20, 64u * 1024u));
  oasis_lf_set_master_time(5000);
  m68k_execute(0x4e71u, 0x400u, 0x402u);
  oasis_lf_set_master_time(3000);
  uint8_t nop[] = {0x00};
  oasis_lf_z80_instruction(0x0050, 0x0051, nop, 1);
  oasis_lf_record m68k_rec{}, z80_rec{};
  assert(oasis_lf_ring_record(1, &m68k_rec));
  assert(oasis_lf_ring_record(2, &z80_rec));
  assert(m68k_rec.stream_sequence < z80_rec.stream_sequence);
  assert(m68k_rec.master_time > z80_rec.master_time);
}

/* Test V: Deterministic output. */
static void test_deterministic()
{
  auto run = []() {
    assert(oasis_lf_configure(1, 20, 64u * 1024u));
    oasis_lf_set_master_time(100);
    m68k_execute(0x4e71u, 0x500u, 0x502u);
    uint8_t ld[] = {0x3E, 0x42};
    oasis_lf_set_master_time(200);
    oasis_lf_z80_instruction(0x0060, 0x0062, ld, 2);
    oasis_lf_record records[2]{};
    assert(oasis_lf_ring_record(1, &records[0]));
    assert(oasis_lf_ring_record(2, &records[1]));
    return std::make_pair(records[0], records[1]);
  };
  auto [r0a, r1a] = run();
  auto [r0b, r1b] = run();
  assert(std::memcmp(&r0a, &r0b, sizeof(oasis_lf_record)) == 0);
  assert(std::memcmp(&r1a, &r1b, sizeof(oasis_lf_record)) == 0);
}

/* Test: Z80 instruction_sequence is independent of M68K. */
static void test_z80_independent_sequence()
{
  assert(oasis_lf_configure(1, 20, 64u * 1024u));
  m68k_execute(0x4e71u, 0x600u, 0x602u);
  m68k_execute(0x4e71u, 0x602u, 0x604u);
  uint8_t nop[] = {0x00};
  oasis_lf_z80_instruction(0x0070, 0x0071, nop, 1);
  oasis_lf_record m0{}, m1{}, z0{};
  assert(oasis_lf_ring_record(1, &m0));
  assert(oasis_lf_ring_record(2, &m1));
  assert(oasis_lf_ring_record(3, &z0));
  assert(m0.instruction_sequence == 1 && m0.cpu_id == OASIS_LF_CPU_68K);
  assert(m1.instruction_sequence == 2 && m1.cpu_id == OASIS_LF_CPU_68K);
  assert(z0.instruction_sequence == 1 && z0.cpu_id == OASIS_LF_CPU_Z80);
}

/* Test: Record version and size. */
static void test_record_format()
{
  assert(sizeof(oasis_lf_record) == 48);
  assert(OASIS_LF_RECORD_VERSION == 2);
}

int main()
{
  test_record_format();
  test_z80_pc_exact();
  test_z80_normal_opcode();
  test_z80_cb_prefix();
  test_z80_ed_prefix();
  test_z80_dd_fd_prefix();
  test_z80_ddcb_fdcb_prefix();
  test_z80_fetch_not_data_read();
  test_z80_ram_read();
  test_z80_ram_write();
  test_bank_register_change();
  test_banked_rom_physical_address();
  test_ym2612_address_write();
  test_ym2612_data_write();
  test_ym2612_ordered_register_write();
  test_psg_write();
  test_dac_as_ym2612();
  test_m68k_unchanged();
  test_z80_filter();
  test_frame_identity();
  test_cross_cpu_master_time();
  test_no_false_causal();
  test_deterministic();
  test_z80_independent_sequence();
  return 0;
}
