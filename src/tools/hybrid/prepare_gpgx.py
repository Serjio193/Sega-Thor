"""Install the developer-only bridge and minimal block hook in GPGX."""
import argparse
from pathlib import Path
import shutil


def prepare(root):
    root = root.resolve()
    coverage = root / "core/debug/coverage.c"
    text = coverage.read_text(encoding="utf-8")
    anchor = '  const char *directory = getenv("GPGX_COVERAGE_DIR");'
    guard = '  if (getenv("GPGX_HYBRID_ONLY")) return; /* M11.28: no coverage/UI */'
    if guard not in text:
        if text.count(anchor) != 1:
            raise ValueError("unsupported external coverage initialization")
        coverage.write_text(text.replace(anchor, anchor + "\n" + guard), encoding="utf-8")
    hook_header = root / "core/debug/cpuhook.h"
    hook_text = hook_header.read_text(encoding="utf-8")
    hook_anchor = "void set_cpu_hook(void(*hook)(hook_type_t type, int width, unsigned int address, unsigned int value));"
    hook_decl = "typedef int (*cpu_block_hook_t)(unsigned int pc);\nextern cpu_block_hook_t cpu_block_hook;\nvoid set_cpu_block_hook(cpu_block_hook_t hook);"
    if "cpu_block_hook_t" not in hook_text:
        if hook_text.count(hook_anchor) != 1:
            raise ValueError("unsupported external CPU hook header")
        hook_header.write_text(hook_text.replace(hook_anchor, hook_anchor + "\n" + hook_decl), encoding="utf-8")
    hook_text = hook_header.read_text(encoding="utf-8")
    helper_decl = """unsigned int m68k_hybrid_fetch16(void);
unsigned int m68k_hybrid_read(unsigned int address, int width);
void m68k_hybrid_write(unsigned int address, int width, unsigned int value);
void m68k_hybrid_begin_instruction(unsigned int opcode);
void m68k_hybrid_finish_instruction(unsigned int opcode);
unsigned int m68k_hybrid_instruction_cycles(unsigned int opcode);
unsigned int m68k_hybrid_cpu_field(unsigned int field);
unsigned int m68k_hybrid_refresh_period(void);
unsigned int m68k_hybrid_refresh_penalty(void);"""
    if "m68k_hybrid_fetch16" not in hook_text:
        hook_header.write_text(hook_text.replace("void set_cpu_block_hook(cpu_block_hook_t hook);", "void set_cpu_block_hook(cpu_block_hook_t hook);\n" + helper_decl), encoding="utf-8")
    hook_source = root / "core/debug/cpuhook.c"
    source_text = hook_source.read_text(encoding="utf-8")
    source_anchor = "void set_cpu_hook(void(*hook)(hook_type_t type, int width, unsigned int address, unsigned int value))\n{\n\tcpu_hook = hook;\n}"
    block_source = "cpu_block_hook_t cpu_block_hook = NULL;\n\nvoid set_cpu_block_hook(cpu_block_hook_t hook)\n{\n\tcpu_block_hook = hook;\n}"
    if "cpu_block_hook_t cpu_block_hook" not in source_text:
        if source_text.count(source_anchor) != 1:
            raise ValueError("unsupported external CPU hook source")
        hook_source.write_text(source_text.replace(source_anchor, source_anchor + "\n\n" + block_source), encoding="utf-8")
    cpu_source = root / "core/m68k/m68kcpu.c"
    cpu_text = cpu_source.read_text(encoding="utf-8")
    cpu_anchor = "#ifdef HOOK_CPU\n    /* Trigger execution hook */\n    if (UNLIKELY(cpu_hook))\n      cpu_hook(HOOK_M68K_E, 0, REG_PC, 0);\n#endif"
    cpu_replacement = "#ifdef HOOK_CPU\n    /* M11.32: translated blocks own their exact fetch/step/cycle path. */\n    int hybrid_block_executed = 0;\n    if (UNLIKELY(cpu_block_hook))\n      hybrid_block_executed = cpu_block_hook(REG_PC);\n    if (!hybrid_block_executed && UNLIKELY(cpu_hook))\n      cpu_hook(HOOK_M68K_E, 0, REG_PC, 0);\n    if (hybrid_block_executed)\n      continue;\n#endif"
    if "hybrid_block_executed" not in cpu_text:
        if cpu_text.count(cpu_anchor) != 1:
            raise ValueError("unsupported external CPU execution loop")
        cpu_source.write_text(cpu_text.replace(cpu_anchor, cpu_replacement), encoding="utf-8")
    cpu_text = cpu_source.read_text(encoding="utf-8")
    helper_anchor = "void m68k_clear_halt(void)\n{\n  /* Clear the HALT line on the CPU */\n  CPU_STOPPED &= ~STOP_LEVEL_HALT;\n}"
    helper_code = r'''void m68k_hybrid_begin_instruction(unsigned int opcode)
{
  REG_IR = opcode & 0xffff;
  if (m68k.cycles >= m68k.refresh_cycles)
  {
    m68k.refresh_cycles = m68k.cycles + (128 * 7);
    m68k.cycles += (2 * 7);
  }
}

void m68k_hybrid_finish_instruction(unsigned int opcode)
{
  REG_IR = opcode & 0xffff;
  USE_CYCLES(CYC_INSTRUCTION[REG_IR]);
}

unsigned int m68k_hybrid_fetch16(void)
{
  return m68ki_read_imm_16();
}

unsigned int m68k_hybrid_read(unsigned int address, int width)
{
  if (width == 1) return m68ki_read_8(address);
  if (width == 2) return m68ki_read_16(address);
  if (width == 4) return m68ki_read_32(address);
  return 0;
}

void m68k_hybrid_write(unsigned int address, int width, unsigned int value)
{
  if (width == 1) m68ki_write_8(address, value);
  else if (width == 2) m68ki_write_16(address, value);
  else if (width == 4) m68ki_write_32(address, value);
}

unsigned int m68k_hybrid_instruction_cycles(unsigned int opcode)
{
  const unsigned int saved = REG_IR;
  unsigned int cycles;
  REG_IR = opcode & 0xffff;
  cycles = CYC_INSTRUCTION[REG_IR];
#ifdef M68K_OVERCLOCK_SHIFT
  cycles = (cycles * m68k.cycle_ratio) >> M68K_OVERCLOCK_SHIFT;
#endif
  REG_IR = saved;
  return cycles;
}

unsigned int m68k_hybrid_cpu_field(unsigned int field)
{
  switch (field)
  {
  case 18: return REG_IR;
#if M68K_EMULATE_PREFETCH
  case 19: return CPU_PREF_ADDR;
  case 20: return CPU_PREF_DATA;
#else
  case 19: case 20: return 0;
#endif
  case 21: return m68k.cycles;
  case 22: return m68k.refresh_cycles;
  default: return 0;
  }
}

unsigned int m68k_hybrid_refresh_period(void) { return 128 * 7; }
unsigned int m68k_hybrid_refresh_penalty(void) { return 2 * 7; }
'''
    if "m68k_hybrid_begin_instruction" not in cpu_text:
        if cpu_text.count(helper_anchor) != 1:
            raise ValueError("unsupported external m68k halt helper anchor")
        cpu_source.write_text(cpu_text.replace(helper_anchor, helper_anchor + "\n\n" + helper_code), encoding="utf-8")
    shutil.copyfile(Path(__file__).with_name("gpgx_bridge.c"), root / "core/debug/gpgx_bridge.c")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("gpgx_source", type=Path)
    prepare(parser.parse_args().gpgx_source)
