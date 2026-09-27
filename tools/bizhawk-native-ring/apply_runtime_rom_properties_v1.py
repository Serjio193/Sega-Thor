"""Apply the live ROM-property hooks to the pinned BizHawk/GPGX checkout."""

from __future__ import annotations

import argparse
import shutil
import subprocess
from pathlib import Path


PINNED = {
    ".": "bdddf4a58aa1a022afb11dc73294a81a5aa7bbd5",
    "waterbox/gpgx/Genesis-Plus-GX": "051d430d3d1b54625f9900c8f152d7f232e06daf",
}
SOURCES = (
    "rom_properties_core.c",
    "rom_properties_core.h",
    "rom_properties_runtime.c",
    "rom_properties_runtime.h",
)


def replace_once(path: Path, old: str, new: str) -> None:
    raw = path.read_bytes()
    newline = b"\r\n" if b"\r\n" in raw else b"\n"
    content = raw.decode("utf-8").replace("\r\n", "\n")
    count = content.count(old)
    if count != 1:
        raise RuntimeError(f"expected one patch anchor in {path}, got {count}")
    updated = content.replace(old, new, 1)
    path.write_bytes(updated.replace("\n", newline.decode()).encode("utf-8"))


def insert_before_final(path: Path, marker: str, addition: str) -> None:
    raw = path.read_bytes()
    newline = b"\r\n" if b"\r\n" in raw else b"\n"
    content = raw.decode("utf-8").replace("\r\n", "\n")
    index = content.rfind(marker)
    if index < 0:
        raise RuntimeError(f"missing final insertion point in {path}")
    content = content[:index] + addition + content[index:]
    path.write_bytes(content.replace("\n", newline.decode()).encode("utf-8"))


def replace_first(path: Path, old: str, new: str) -> None:
    raw = path.read_bytes()
    newline = b"\r\n" if b"\r\n" in raw else b"\n"
    content = raw.decode("utf-8").replace("\r\n", "\n")
    if old not in content:
        raise RuntimeError(f"missing patch anchor in {path}")
    content = content.replace(old, new, 1)
    path.write_bytes(content.replace("\n", newline.decode()).encode("utf-8"))


def git_head(root: Path, subdir: str) -> str:
    return subprocess.check_output(
        ["git", "-C", str(root / subdir), "rev-parse", "HEAD"], text=True
    ).strip()


def apply(root: Path, project: Path) -> None:
    for subdir, commit in PINNED.items():
        if git_head(root, subdir) != commit:
            raise RuntimeError(f"unexpected pinned checkout: {subdir}")
    gx = root / "waterbox/gpgx/Genesis-Plus-GX"
    debug = gx / "core/debug"
    debug.mkdir(parents=True, exist_ok=True)
    for name in SOURCES:
        source = project / "tools/bizhawk-native-ring" / name
        destination = debug / name
        if destination.exists() and destination.read_bytes() != source.read_bytes():
            raise RuntimeError(f"refusing to replace changed file: {destination}")
        shutil.copyfile(source, destination)

    replace_once(root / "waterbox/gpgx/Makefile",
        "\t$(GPGX_DIR)/core/debug/live_forward_flow.c \\\n\tcinterface/live_forward_api.c \\",
        "\t$(GPGX_DIR)/core/debug/live_forward_flow.c \\\n"
        "\t$(GPGX_DIR)/core/debug/rom_properties_core.c \\\n"
        "\t$(GPGX_DIR)/core/debug/rom_properties_runtime.c \\\n"
        "\tcinterface/live_forward_api.c \\")

    replace_once(root / "waterbox/gpgx/cinterface/live_forward_api.c",
        '#include <debug/live_forward_trace.h>',
        '#include <debug/live_forward_trace.h>\n'
        '#include <debug/rom_properties_runtime.h>')
    replace_once(root / "waterbox/gpgx/cinterface/live_forward_api.c",
        "GPGX_EX int gpgx_live_forward_set_ring_capacity(uint32_t capacity)\n"
        "{\n\treturn oasis_lf_set_ring_capacity(capacity);\n}\n",
        "GPGX_EX int gpgx_live_forward_set_ring_capacity(uint32_t capacity)\n"
        "{\n\treturn oasis_lf_set_ring_capacity(capacity);\n}\n\n"
        "GPGX_EX void gpgx_rom_properties_enable(uint32_t enabled)\n"
        "{\n\toasis_romprops_enable(enabled);\n}\n\n"
        "GPGX_EX int gpgx_rom_properties_clear(void)\n"
        "{\n\treturn oasis_romprops_clear();\n}\n\n"
        "GPGX_EX uint32_t gpgx_rom_properties_size(void)\n"
        "{\n\treturn oasis_romprops_size();\n}\n\n"
        "GPGX_EX uint64_t gpgx_rom_properties_changed_bytes(void)\n"
        "{\n\treturn oasis_romprops_changed_bytes();\n}\n\n"
        "GPGX_EX uint64_t gpgx_rom_properties_change_operations(void)\n"
        "{\n\treturn oasis_romprops_change_operations();\n}\n\n"
        "GPGX_EX uint32_t gpgx_rom_properties_copy(uint32_t offset,\n"
        "\tuint16_t *output, uint32_t capacity)\n"
        "{\n\treturn oasis_romprops_copy(offset, output, capacity);\n}\n\n"
        "GPGX_EX uint32_t gpgx_rom_properties_copy_dirty(uint32_t first_page,\n"
        "\tuint8_t *output, uint32_t capacity)\n"
        "{\n\treturn oasis_romprops_copy_dirty(first_page, output, capacity);\n}\n")

    managed_root = root / "src/BizHawk.Emulation.Cores/Consoles/Sega/gpgx64"
    replace_once(managed_root / "LibGPGX.cs",
        "\t\tpublic abstract uint gpgx_live_forward_result_copy(uint workerId, ulong generation, uint offset, [Out] LiveForwardRecord[] output, uint capacity);",
        "\t\tpublic abstract uint gpgx_live_forward_result_copy(uint workerId, ulong generation, uint offset, [Out] LiveForwardRecord[] output, uint capacity);\n\n"
        "\t\t[BizImport(CallingConvention.Cdecl)]\n"
        "\t\tpublic abstract void gpgx_rom_properties_enable(uint enabled);\n\n"
        "\t\t[BizImport(CallingConvention.Cdecl)]\n"
        "\t\tpublic abstract int gpgx_rom_properties_clear();\n\n"
        "\t\t[BizImport(CallingConvention.Cdecl)]\n"
        "\t\tpublic abstract uint gpgx_rom_properties_size();\n\n"
        "\t\t[BizImport(CallingConvention.Cdecl)]\n"
        "\t\tpublic abstract ulong gpgx_rom_properties_changed_bytes();\n\n"
        "\t\t[BizImport(CallingConvention.Cdecl)]\n"
        "\t\tpublic abstract ulong gpgx_rom_properties_change_operations();\n\n"
        "\t\t[BizImport(CallingConvention.Cdecl)]\n"
        "\t\tpublic abstract uint gpgx_rom_properties_copy(uint offset, [Out] ushort[] output, uint capacity);")
    shutil.copyfile(project / "tools/bizhawk-native-ring/GPGX.LiveRomProperties.cs",
                    managed_root / "GPGX.LiveRomProperties.cs")

    lua_library = root / "src/BizHawk.Client.Common/lua/LuaHelperLibs/GenesisLuaLibrary.cs"
    insert_before_final(lua_library, "\t}\n}",
        "\t\t[LuaMethod(\"rom_properties_enable\", \"Enables or disables live ROM property observation\")]\n"
        "\t\tpublic bool RomPropertiesEnable(bool enabled)\n"
        "\t\t\t=> Emulator is GPGX gpgx && gpgx.RomPropertiesEnable(enabled);\n\n"
        "\t\t[LuaMethod(\"rom_properties_clear\", \"Starts a fresh live ROM property run\")]\n"
        "\t\tpublic bool RomPropertiesClear()\n"
        "\t\t\t=> Emulator is GPGX gpgx && gpgx.RomPropertiesClear();\n\n"
        "\t\t[LuaMethod(\"rom_properties_size\", \"Returns the loaded ROM property map size\")]\n"
        "\t\tpublic long RomPropertiesSize()\n"
        "\t\t\t=> Emulator is GPGX gpgx ? gpgx.RomPropertiesSize() : 0;\n\n"
        "\t\t[LuaMethod(\"rom_properties_changed_bytes\", \"Returns the count of bytes with at least one observed property\")]\n"
        "\t\tpublic long RomPropertiesChangedBytes()\n"
        "\t\t\t=> Emulator is GPGX gpgx ? checked((long)gpgx.RomPropertiesChangedBytes()) : 0;\n\n"
        "\t\t[LuaMethod(\"rom_properties_change_operations\", \"Returns effective property-bit additions\")]\n"
        "\t\tpublic long RomPropertiesChangeOperations()\n"
        "\t\t\t=> Emulator is GPGX gpgx ? checked((long)gpgx.RomPropertiesChangeOperations()) : 0;\n\n"
        "\t\t[LuaMethod(\"rom_properties_copy_hex\", \"Copies a bounded property-map chunk as four hex digits per ROM byte\")]\n"
        "\t\tpublic string RomPropertiesCopyHex(long offset, int count)\n"
        "\t\t{\n"
        "\t\t\tif (Emulator is not GPGX gpgx || offset < 0 || count < 1 || count > 4096) return string.Empty;\n"
        "\t\t\treturn gpgx.RomPropertiesCopyHex((uint)offset, (uint)count);\n"
        "\t\t}\n\n")

    replace_once(gx / "core/loadrom.c", '#include "shared.h"',
        '#include "shared.h"\n#ifdef BIZHAWK_LIVE_FORWARD_WORKER\n'
        '#include "debug/rom_properties_runtime.h"\n#endif')
    replace_once(gx / "core/loadrom.c", "  cart.romsize = size;",
        "  cart.romsize = size;\n#ifdef BIZHAWK_LIVE_FORWARD_WORKER\n"
        "  oasis_romprops_init(cart.romsize);\n#endif")

    replace_once(gx / "core/m68k/m68kcpu.h",
        '#include "../debug/live_forward_trace.h"',
        '#include "../debug/live_forward_trace.h"\n'
        '#include "../debug/rom_properties_runtime.h"')
    replace_once(gx / "core/m68k/m68kcpu.h",
        "#define m68ki_live_forward_unwind() oasis_lf_unwind()",
        "#define m68ki_live_forward_unwind() do { \\\n"
        "  oasis_lf_unwind(); oasis_romprops_m68k_abort(); \\\n"
        "} while (0)")
    replace_once(gx / "core/m68k/m68kcpu.h",
        "#define m68ki_live_forward_unwind()\n#endif",
        "#define m68ki_live_forward_unwind()\n"
        "#define oasis_romprops_m68k_abort()\n"
        "#define oasis_romprops_m68k_prefetch(A, B)\n"
        "#define oasis_romprops_m68k_consume_word(A, B)\n"
        "#define oasis_romprops_m68k_data_read(A, B, C)\n#endif")

    replace_once(gx / "core/m68k/m68kcpu.h",
        "    CPU_PREF_DATA = m68k_read_immediate_16(REG_PC);\n  }\n  temp_val = CPU_PREF_DATA;\n  REG_PC += 2;\n  CPU_PREF_ADDR = REG_PC;\n  CPU_PREF_DATA = m68k_read_immediate_16(REG_PC);\n  return temp_val;",
        "    CPU_PREF_DATA = m68k_read_immediate_16(REG_PC);\n"
        "    oasis_romprops_m68k_prefetch(REG_PC, (uint16_t)CPU_PREF_DATA);\n"
        "  }\n  temp_val = CPU_PREF_DATA;\n"
        "  oasis_romprops_m68k_consume_word(REG_PC, (uint16_t)temp_val);\n"
        "  REG_PC += 2;\n  CPU_PREF_ADDR = REG_PC;\n"
        "  CPU_PREF_DATA = m68k_read_immediate_16(REG_PC);\n"
        "  oasis_romprops_m68k_prefetch(REG_PC, (uint16_t)CPU_PREF_DATA);\n"
        "  return temp_val;")
    replace_once(gx / "core/m68k/m68kcpu.h",
        "  REG_PC += 2;\n  return m68k_read_immediate_16(pc);\n#endif /* M68K_EMULATE_PREFETCH */",
        "  REG_PC += 2;\n  uint value = m68k_read_immediate_16(pc);\n"
        "#ifdef BIZHAWK_LIVE_FORWARD_WORKER\n"
        "  oasis_romprops_m68k_prefetch(pc, (uint16_t)value);\n"
        "  oasis_romprops_m68k_consume_word(pc, (uint16_t)value);\n"
        "#endif\n  return value;\n#endif /* M68K_EMULATE_PREFETCH */")

    replace_once(gx / "core/m68k/m68kcpu.h",
        "  temp_val = CPU_PREF_DATA;\n  REG_PC += 2;\n"
        "  temp_val = (temp_val << 16) | m68k_read_immediate_16(REG_PC);\n"
        "  REG_PC += 2;\n  CPU_PREF_ADDR = REG_PC;\n"
        "  CPU_PREF_DATA = m68k_read_immediate_16(REG_PC);\n  return temp_val;",
        "  temp_val = CPU_PREF_DATA;\n"
        "  oasis_romprops_m68k_consume_word(REG_PC, (uint16_t)temp_val);\n"
        "  REG_PC += 2;\n  uint second_pc = REG_PC;\n"
        "  uint second_word = m68k_read_immediate_16(second_pc);\n"
        "  oasis_romprops_m68k_prefetch(second_pc, (uint16_t)second_word);\n"
        "  oasis_romprops_m68k_consume_word(second_pc, (uint16_t)second_word);\n"
        "  temp_val = (temp_val << 16) | second_word;\n"
        "  REG_PC += 2;\n  CPU_PREF_ADDR = REG_PC;\n"
        "  CPU_PREF_DATA = m68k_read_immediate_16(REG_PC);\n"
        "  oasis_romprops_m68k_prefetch(REG_PC, (uint16_t)CPU_PREF_DATA);\n"
        "  return temp_val;")
    replace_once(gx / "core/m68k/m68kcpu.h",
        "  REG_PC += 4;\n  return m68k_read_immediate_32(pc);\n#endif /* M68K_EMULATE_PREFETCH */",
        "  REG_PC += 4;\n  uint value = m68k_read_immediate_32(pc);\n"
        "  uint first_word = (value >> 16) & 0xffffu;\n"
        "  uint second_word = value & 0xffffu;\n"
        "  oasis_romprops_m68k_prefetch(pc, (uint16_t)first_word);\n"
        "  oasis_romprops_m68k_consume_word(pc, (uint16_t)first_word);\n"
        "  oasis_romprops_m68k_prefetch(pc + 2u, (uint16_t)second_word);\n"
        "  oasis_romprops_m68k_consume_word(pc + 2u, (uint16_t)second_word);\n"
        "  return value;\n#endif /* M68K_EMULATE_PREFETCH */")

    for width in (8, 16):
        anchor = (f"    oasis_lf_bus_read(address, val, {width},\n"
            "      oasis_lf_resolve_domain(temp, address));\n#endif\n\n  return val;")
        replacement = (f"    oasis_lf_bus_read(address, val, {width},\n"
            "      oasis_lf_resolve_domain(temp, address));\n"
            f"  oasis_romprops_m68k_data_read(address, val, {width}u);\n"
            "#endif\n\n  return val;")
        replace_once(gx / "core/m68k/m68kcpu.h", anchor, replacement)
    replace_once(gx / "core/m68k/m68kcpu.h",
        "    oasis_lf_bus_read(address, val, 32,\n"
        "      domain0 == domain1 ? domain0 : OASIS_LF_BUS_OTHER);\n#endif\n\n  return val;",
        "    oasis_lf_bus_read(address, val, 32,\n"
        "      domain0 == domain1 ? domain0 : OASIS_LF_BUS_OTHER);\n"
        "  oasis_romprops_m68k_data_read(address, val, 32u);\n"
        "#endif\n\n  return val;")

    replace_once(gx / "core/m68k/m68kcpu.c",
        "\tif (!oasis_lf_recording_enabled)\n\t\treturn;",
        "\toasis_romprops_m68k_abort();\n"
        "\tif (!oasis_lf_recording_enabled)\n\t\treturn;")
    replace_once(gx / "core/m68k/m68kcpu.c",
        "      m68ki_use_data_space() /* auto-disable (see m68kcpu.h) */\n"
        "#ifdef BIZHAWK_LIVE_FORWARD_WORKER",
        "      m68ki_use_data_space() /* auto-disable (see m68kcpu.h) */\n"
        "#ifdef BIZHAWK_LIVE_FORWARD_WORKER\n"
        "      oasis_romprops_m68k_begin();\n"
        "#endif\n"
        "#ifdef BIZHAWK_LIVE_FORWARD_WORKER")

    replace_once(gx / "core/memz80.c", '#include "debug/live_forward_trace.h"',
        '#include "debug/live_forward_trace.h"\n'
        '#include "debug/rom_properties_runtime.h"')
    replace_once(gx / "core/memz80.c",
        "      unsigned char val = READ_BYTE(m68k.memory_map[address >> 16].base, address & 0xFFFF);\n"
        "#ifdef BIZHAWK_LIVE_FORWARD_WORKER\n"
        "      oasis_lf_z80_bus_read(Z80.pc.w.l, (uint16_t)(address & 0x7FFFu) | 0x8000u,\n"
        "                            val, 8, OASIS_LF_BUS_BANKED_ROM, address);\n"
        "#endif\n      return val;",
        "      unsigned char val = READ_BYTE(m68k.memory_map[address >> 16].base, address & 0xFFFF);\n"
        "#ifdef BIZHAWK_LIVE_FORWARD_WORKER\n"
        "      oasis_lf_z80_bus_read(Z80.pc.w.l, (uint16_t)(address & 0x7FFFu) | 0x8000u,\n"
        "                            val, 8, OASIS_LF_BUS_BANKED_ROM, address);\n"
        "      oasis_romprops_z80_data_read(address, val);\n"
        "#endif\n      return val;")
    replace_once(gx / "core/vdp_ctrl.c", '#include "shared.h"',
        '#include "shared.h"\n#ifdef BIZHAWK_LIVE_FORWARD_WORKER\n'
        '#include "debug/rom_properties_runtime.h"\n#endif')
    replace_first(gx / "core/vdp_ctrl.c",
        "  do\n  {\n    /* Read data word from 68k bus */",
        "  do\n  {\n    uint32 source_address = source;\n\n"
        "    /* Read data word from 68k bus */")
    replace_first(gx / "core/vdp_ctrl.c",
        "    /* Write data word to VRAM, CRAM or VSRAM */\n    vdp_bus_w(data);",
        "    /* Write data word to VRAM, CRAM or VSRAM */\n    vdp_bus_w(data);\n"
        "#ifdef BIZHAWK_LIVE_FORWARD_WORKER\n"
        "    oasis_romprops_vdp_dma_source(source_address, data, code & 0x0fu);\n"
        "#endif")
    replace_once(gx / "core/vdp_ctrl.c",
        "  /* Write data */\n  vdp_bus_w(data);\n\n"
        "  /* Check if DMA Fill is pending */",
        "  /* Write data */\n  vdp_bus_w(data);\n"
        "#ifdef BIZHAWK_LIVE_FORWARD_WORKER\n"
        "  oasis_romprops_vdp_cpu_source_write((uint16_t)data,\n"
        "    code & 0x0fu, (uint32_t)(dmafill != 0));\n"
        "#endif\n\n"
        "  /* Check if DMA Fill is pending */")
    replace_once(gx / "core/m68k/m68kcpu.c",
        "      m68ki_exception_if_trace() /* auto-disable (see m68kcpu.h) */\n"
        "#ifdef BIZHAWK_LIVE_FORWARD_WORKER",
        "      m68ki_exception_if_trace() /* auto-disable (see m68kcpu.h) */\n"
        "#ifdef BIZHAWK_LIVE_FORWARD_WORKER\n"
        "      oasis_romprops_m68k_finish(CPU_STOPPED == 0);\n"
        "#endif\n"
        "#ifdef BIZHAWK_LIVE_FORWARD_WORKER")
    replace_once(gx / "core/m68k/m68kcpu.c",
        "  while (m68k.cycles < cycles)\n  {\n#ifdef BIZHAWK_LIVE_FORWARD_WORKER",
        "  while (m68k.cycles < cycles)\n  {\n"
        "#ifdef BIZHAWK_LIVE_FORWARD_WORKER\n"
        "    oasis_romprops_m68k_begin();\n"
        "#endif\n"
        "#ifdef BIZHAWK_LIVE_FORWARD_WORKER")
    replace_once(gx / "core/m68k/m68kcpu.c",
        "    m68ki_exception_if_trace(); /* auto-disable (see m68kcpu.h) */\n"
        "#ifdef BIZHAWK_LIVE_FORWARD_WORKER",
        "    m68ki_exception_if_trace(); /* auto-disable (see m68kcpu.h) */\n"
        "#ifdef BIZHAWK_LIVE_FORWARD_WORKER\n"
        "    oasis_romprops_m68k_finish(CPU_STOPPED == 0);\n"
        "#endif\n"
        "#ifdef BIZHAWK_LIVE_FORWARD_WORKER")

    replace_once(gx / "core/z80/z80.c", '#include "../debug/live_forward_trace.h"',
        '#include "../debug/live_forward_trace.h"\n'
        '#include "../debug/rom_properties_runtime.h"')
    replace_once(gx / "core/z80/z80.c",
        "  z80_last_fetch = cpu_readop(pc);\n  return z80_last_fetch;",
        "  z80_last_fetch = cpu_readop(pc);\n"
        "  oasis_romprops_z80_fetch((uint16_t)pc,\n"
        "    &z80_readmap[pc >> 10][pc & 0x03FF], z80_last_fetch);\n"
        "  return z80_last_fetch;")
    replace_once(gx / "core/z80/z80.c",
        "  return cpu_readop_arg(pc);\n}\n\nINLINE UINT32 ARG16(void)",
        "  UINT8 value = cpu_readop_arg(pc);\n"
        "  oasis_romprops_z80_fetch((uint16_t)pc,\n"
        "    &z80_readmap[pc >> 10][pc & 0x03FF], value);\n"
        "  return value;\n}\n\nINLINE UINT32 ARG16(void)")
    replace_once(gx / "core/z80/z80.c",
        "  PC += 2;\n  return cpu_readop_arg(pc) | (cpu_readop_arg((pc+1)&0xffff) << 8);",
        "  UINT8 low = cpu_readop_arg(pc);\n"
        "  UINT8 high = cpu_readop_arg((pc + 1u) & 0xffffu);\n"
        "  oasis_romprops_z80_fetch((uint16_t)pc,\n"
        "    &z80_readmap[pc >> 10][pc & 0x03FF], low);\n"
        "  oasis_romprops_z80_fetch((uint16_t)((pc + 1u) & 0xffffu),\n"
        "    &z80_readmap[((pc + 1u) & 0xffffu) >> 10]\n"
        "      [(pc + 1u) & 0x03FF], high);\n"
        "  PC += 2;\n"
        "  return low | ((UINT32)high << 8);")
    replace_once(gx / "core/z80/z80.c",
        "    EXEC_INLINE(op,ROP());\n#ifdef BIZHAWK_LIVE_FORWARD_WORKER",
        "#ifdef BIZHAWK_LIVE_FORWARD_WORKER\n"
        "    oasis_romprops_z80_begin();\n"
        "#endif\n"
        "    EXEC_INLINE(op,ROP());\n"
        "#ifdef BIZHAWK_LIVE_FORWARD_WORKER\n"
        "    oasis_romprops_z80_finish(1u);\n"
        "#endif\n"
        "#ifdef BIZHAWK_LIVE_FORWARD_WORKER")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--bizhawk-root", type=Path, required=True)
    parser.add_argument("--project-root", type=Path, required=True)
    args = parser.parse_args()
    apply(args.bizhawk_root.resolve(), args.project_root.resolve())


if __name__ == "__main__":
    main()
