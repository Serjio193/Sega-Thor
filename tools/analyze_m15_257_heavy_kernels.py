#!/usr/bin/env python3
"""Analyze heavy kernels 0x00C8AA and 0x00D406 from canonical ROM."""
import struct
import pathlib

def load_rom():
    p = pathlib.Path("local-roms/Beyond Oasis (USA).md")
    return p.read_bytes()

def disasm_range(rom, start_pc, max_words=64):
    pc = start_pc
    words = []
    lines = []
    while pc < start_pc + max_words * 2 and pc + 2 <= len(rom):
        w = struct.unpack_from(">H", rom, pc)[0]
        words.append(w)
        line = f"{pc:06X}: {w:04X}"
        if w == 0x4E75: # RTS
            line += "  rts"
            lines.append(line)
            pc += 2
            break
        elif w == 0x4E73: # RTE
            line += "  rte"
            lines.append(line)
            pc += 2
            break
        elif (w & 0xF000) == 0x6000: # Bcc / BRA / BSR
            cond = (w >> 8) & 0x0F
            disp = w & 0xFF
            op = "bra" if cond == 0 else ("bsr" if cond == 1 else f"b{cond:x}")
            if disp == 0:
                ext = struct.unpack_from(">h", rom, pc + 2)[0]
                tgt = pc + 2 + ext
                line += f" {struct.unpack_from('>H', rom, pc+2)[0]:04X}  {op}.w  ${tgt:06X}"
                pc += 4
            else:
                if disp >= 0x80: disp -= 0x100
                tgt = pc + 2 + disp
                line += f"       {op}.s  ${tgt:06X}"
                pc += 2
        elif (w & 0xF1F8) == 0x51C8: # DBRA
            reg = w & 7
            disp = struct.unpack_from(">h", rom, pc + 2)[0]
            tgt = pc + 2 + disp
            ext = struct.unpack_from(">H", rom, pc + 2)[0]
            line += f" {ext:04X}  dbra  d{reg}, ${tgt:06X}"
            pc += 4
        else:
            line += "  ..."
            pc += 2
        lines.append(line)
    return lines, pc

def find_callers(rom, target_addr):
    callers = []
    for pc in range(0, min(len(rom), 0x100000), 2):
        w = struct.unpack_from(">H", rom, pc)[0]
        # JSR $addr.l -> 4EB9 xxxxxxxx
        if w == 0x4EB9 and pc + 6 <= len(rom):
            addr = struct.unpack_from(">I", rom, pc + 2)[0]
            if addr == target_addr:
                callers.append((pc, "jsr.l"))
        # BSR.W -> 6100 xxxx
        elif w == 0x6100 and pc + 4 <= len(rom):
            disp = struct.unpack_from(">h", rom, pc + 2)[0]
            if pc + 2 + disp == target_addr:
                callers.append((pc, "bsr.w"))
        # BSR.S -> 61xx
        elif (w & 0xFF00) == 0x6100 and (w & 0xFF) != 0:
            disp = w & 0xFF
            if disp >= 0x80: disp -= 0x100
            if pc + 2 + disp == target_addr:
                callers.append((pc, "bsr.s"))
    return callers

def main():
    rom = load_rom()
    print("=== Analyzing 0x00C8AA ===")
    lines, end_pc = disasm_range(rom, 0x00C8AA, max_words=100)
    for l in lines:
        print("  ", l)
    print(f"Entry: 0x00C8AA, Potential Exit: 0x{end_pc:06X}, Length: {end_pc - 0x00C8AA} bytes")
    callers_c8aa = find_callers(rom, 0x00C8AA)
    print(f"Callers of 0x00C8AA ({len(callers_c8aa)}): {[f'0x{c[0]:06X} ({c[1]})' for c in callers_c8aa]}")

    print("\n=== Analyzing 0x00D406 ===")
    lines2, end_pc2 = disasm_range(rom, 0x00D406, max_words=100)
    for l in lines2:
        print("  ", l)
    print(f"Entry: 0x00D406, Potential Exit: 0x{end_pc2:06X}, Length: {end_pc2 - 0x00D406} bytes")
    callers_d406 = find_callers(rom, 0x00D406)
    print(f"Callers of 0x00D406 ({len(callers_d406)}): {[f'0x{c[0]:06X} ({c[1]})' for c in callers_d406]}")

if __name__ == "__main__":
    main()
