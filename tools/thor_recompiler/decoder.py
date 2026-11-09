#!/usr/bin/env python3
"""
M68000 Instruction Decoder for Thor Recompiler Prototype.
Decodes representative instruction subset for tilemap pipelines.
Strictly <= 500 lines per AGENTS.md Rule 3.
"""
from dataclasses import dataclass, field
import struct
from typing import List, Optional, Tuple

@dataclass
class Operand:
    kind: str  # "reg_d", "reg_a", "imm", "disp_a", "indir_a", "postinc_a", "abs_l", "branch_disp"
    reg: int = 0
    val: int = 0
    size: str = "w"  # "b", "w", "l"

@dataclass
class DecodedInstruction:
    pc: int
    raw_words: List[int]
    mnemonic: str
    size: str
    ops: List[Operand] = field(default_factory=list)
    branch_target: Optional[int] = None
    base_cycles: int = 4
    variable_cycles: int = 0  # e.g. for shifts
    skip_refresh: bool = False

def decode_at(rom: bytes, pc: int) -> DecodedInstruction:
    w0 = struct.unpack_from(">H", rom, pc)[0]
    words = [w0]
    
    # RTS: 0x4E75
    if w0 == 0x4E75:
        return DecodedInstruction(pc, words, "RTS", "", [], base_cycles=16)

    # SWAP: 0x4840 | reg
    if (w0 & 0xFFF8) == 0x4840:
        reg = w0 & 7
        return DecodedInstruction(pc, words, "SWAP", "w", [Operand("reg_d", reg=reg)], base_cycles=4)

    # BRA / Bcc: 0x6000..0x6F00
    if (w0 & 0xF000) == 0x6000:
        cond_codes = {
            0: "BRA", 1: "BSR", 2: "BHI", 3: "BLS", 4: "BCC", 5: "BCS",
            6: "BNE", 7: "BEQ", 8: "BVC", 9: "BVS", 10: "BPL", 11: "BMI",
            12: "BGE", 13: "BLT", 14: "BGT", 15: "BLE"
        }
        cond = (w0 >> 8) & 0x0F
        mnemonic = cond_codes[cond]
        disp8 = struct.unpack(">b", bytes([w0 & 0xFF]))[0]
        if disp8 == 0:
            w1 = struct.unpack_from(">h", rom, pc + 2)[0]
            words.append(w1 & 0xFFFF)
            target = pc + 2 + w1
        else:
            target = pc + 2 + disp8
        return DecodedInstruction(pc, words, mnemonic, "w", [Operand("branch_disp", val=target)], branch_target=target, base_cycles=10)

    # DBcc / DBRA: 0x50C8..0x5FCF ((w0 & 0xF0F8) == 0x50C8)
    if (w0 & 0xF0F8) == 0x50C8:
        reg = w0 & 7
        cond = (w0 >> 8) & 0x0F
        mnemonic = "DBRA" if cond == 1 else f"DB{cond}"
        w1 = struct.unpack_from(">h", rom, pc + 2)[0]
        words.append(w1 & 0xFFFF)
        target = pc + 2 + w1
        return DecodedInstruction(pc, words, mnemonic, "w", [Operand("reg_d", reg=reg), Operand("branch_disp", val=target)], branch_target=target, base_cycles=10)

    # ADDQ / SUBQ: 0x5000 (ADDQ), 0x5100 (SUBQ) - checked AFTER DBRA!
    if (w0 & 0xF100) in (0x5000, 0x5100):
        is_sub = (w0 & 0x0100) != 0
        data = (w0 >> 9) & 7
        imm = 8 if data == 0 else data
        mode = (w0 >> 3) & 7
        reg = w0 & 7
        mnemonic = "SUBQ" if is_sub else "ADDQ"
        if mode == 0:
            return DecodedInstruction(pc, words, mnemonic, "w", [Operand("imm", val=imm), Operand("reg_d", reg=reg)], base_cycles=4)
        elif mode == 1:
            return DecodedInstruction(pc, words, mnemonic, "w", [Operand("imm", val=imm), Operand("reg_a", reg=reg)], base_cycles=8)

    # LEA: 0x41C0..0x4FF8
    if (w0 & 0xF1C0) == 0x41C0:
        a_reg = (w0 >> 9) & 7
        mode = (w0 >> 3) & 7
        reg = w0 & 7
        if mode == 7 and reg == 1: # abs.l (0x45F9 / 0x47F9 / 0x41F9)
            addr = struct.unpack_from(">I", rom, pc + 2)[0]
            words.extend([(addr >> 16) & 0xFFFF, addr & 0xFFFF])
            return DecodedInstruction(pc, words, "LEA", "l", [Operand("abs_l", val=addr), Operand("reg_a", reg=a_reg)], base_cycles=12)
        elif mode == 5: # d16(An) (0x49EA)
            disp = struct.unpack_from(">h", rom, pc + 2)[0]
            words.append(disp & 0xFFFF)
            return DecodedInstruction(pc, words, "LEA", "l", [Operand("disp_a", reg=reg, val=disp), Operand("reg_a", reg=a_reg)], base_cycles=8)

    # MOVE / MOVEA: bits 15..14 == 00, bits 13..12 != 00
    if (w0 & 0xC000) == 0x0000 and (w0 & 0x3000) != 0x0000:
        sz = "b" if (w0 & 0x3000) == 0x1000 else ("l" if (w0 & 0x3000) == 0x2000 else "w")
        dst_reg = (w0 >> 9) & 7
        dst_mode = (w0 >> 6) & 7
        src_mode = (w0 >> 3) & 7
        src_reg = w0 & 7

        # Decode source operand
        offset = 2
        if src_mode == 0:
            src_op = Operand("reg_d", reg=src_reg, size=sz)
        elif src_mode == 1:
            src_op = Operand("reg_a", reg=src_reg, size=sz)
        elif src_mode == 2:
            src_op = Operand("indir_a", reg=src_reg, size=sz)
        elif src_mode == 3:
            src_op = Operand("postinc_a", reg=src_reg, size=sz)
        elif src_mode == 5:
            disp = struct.unpack_from(">h", rom, pc + offset)[0]
            words.append(disp & 0xFFFF)
            offset += 2
            src_op = Operand("disp_a", reg=src_reg, val=disp, size=sz)
        elif src_mode == 7 and src_reg == 4: # imm
            if sz == "l":
                val = struct.unpack_from(">I", rom, pc + offset)[0]
                words.extend([(val >> 16) & 0xFFFF, val & 0xFFFF])
                offset += 4
            else:
                val = struct.unpack_from(">H", rom, pc + offset)[0]
                words.append(val)
                offset += 2
            src_op = Operand("imm", val=val, size=sz)
        else:
            raise NotImplementedError(f"Unsupported MOVE src mode={src_mode} reg={src_reg} at 0x{pc:06X}")

        # Decode dest operand
        if dst_mode == 0:
            dst_op = Operand("reg_d", reg=dst_reg, size=sz)
        elif dst_mode == 1:
            return DecodedInstruction(pc, words, "MOVEA", sz, [src_op, Operand("reg_a", reg=dst_reg, size=sz)], base_cycles=8 if sz=="w" else 12)
        elif dst_mode == 2:
            dst_op = Operand("indir_a", reg=dst_reg, size=sz)
        elif dst_mode == 5:
            disp = struct.unpack_from(">h", rom, pc + offset)[0]
            words.append(disp & 0xFFFF)
            dst_op = Operand("disp_a", reg=dst_reg, val=disp, size=sz)
        else:
            raise NotImplementedError(f"Unsupported MOVE dst mode={dst_mode} reg={dst_reg} at 0x{pc:06X}")

        return DecodedInstruction(pc, words, "MOVE", sz, [src_op, dst_op], base_cycles=8 if sz!="l" else 12)

    # Immediates: ADDI, SUBI, CMPI, ANDI, ORI (0x0000..0x0C00)
    imm_op_types = {0x00: "ORI", 0x02: "ANDI", 0x04: "SUBI", 0x06: "ADDI", 0x0C: "CMPI"}
    type_byte = (w0 >> 8) & 0xFF
    if type_byte in imm_op_types and (w0 & 0x00C0) in (0x0040, 0x0080):
        sz = "w" if (w0 & 0x00C0) == 0x0040 else "l"
        mnemonic = imm_op_types[type_byte]
        if sz == "w":
            imm = struct.unpack_from(">H", rom, pc + 2)[0]
            words.append(imm)
            offset = 4
        else:
            imm = struct.unpack_from(">I", rom, pc + 2)[0]
            words.extend([(imm >> 16) & 0xFFFF, imm & 0xFFFF])
            offset = 6
        dst_mode = (w0 >> 3) & 7
        dst_reg = w0 & 7
        if dst_mode == 0:
            return DecodedInstruction(pc, words, mnemonic, sz, [Operand("imm", val=imm, size=sz), Operand("reg_d", reg=dst_reg, size=sz)], base_cycles=8)
        elif dst_mode == 5:
            disp = struct.unpack_from(">h", rom, pc + offset)[0]
            words.append(disp & 0xFFFF)
            return DecodedInstruction(pc, words, mnemonic, sz, [Operand("imm", val=imm, size=sz), Operand("disp_a", reg=dst_reg, val=disp, size=sz)], base_cycles=16)

    # ADD / SUB / ADDA / SUBA
    if (w0 & 0xF000) in (0xD000, 0x9000):
        is_sub = (w0 & 0xF000) == 0x9000
        reg = (w0 >> 9) & 7
        opmode = (w0 >> 6) & 7
        src_mode = (w0 >> 3) & 7
        src_reg = w0 & 7

        if opmode in (3, 7): # ADDA / SUBA
            sz = "w" if opmode == 3 else "l"
            mnemonic = "SUBA" if is_sub else "ADDA"
            if src_mode == 0:
                return DecodedInstruction(pc, words, mnemonic, sz, [Operand("reg_d", reg=src_reg, size=sz), Operand("reg_a", reg=reg, size="l")], base_cycles=8)
            elif src_mode == 1:
                return DecodedInstruction(pc, words, mnemonic, sz, [Operand("reg_a", reg=src_reg, size=sz), Operand("reg_a", reg=reg, size="l")], base_cycles=8)
        elif opmode == 1: # ADD.W / SUB.W (Dn, <ea>)
            mnemonic = "SUB" if is_sub else "ADD"
            if src_mode == 0: # reg to reg
                return DecodedInstruction(pc, words, mnemonic, "w", [Operand("reg_d", reg=src_reg), Operand("reg_d", reg=reg)], base_cycles=4)

    # CMP
    if (w0 & 0xF000) == 0xB000 and ((w0 >> 6) & 7) == 1:
        d_dst = (w0 >> 9) & 7
        src_mode = (w0 >> 3) & 7
        src_reg = w0 & 7
        if src_mode == 0:
            return DecodedInstruction(pc, words, "CMP", "w", [Operand("reg_d", reg=src_reg), Operand("reg_d", reg=d_dst)], base_cycles=4)

    # Shifts / Rotates: 0xE000
    if (w0 & 0xF000) == 0xE000:
        count_or_reg = (w0 >> 9) & 7
        dr = (w0 >> 8) & 1 # 0: right, 1: left
        ir = (w0 >> 5) & 1 # 0: imm count, 1: reg count
        shift_type = (w0 >> 3) & 3 # 0: ASR/ASL, 1: LSR/LSL, 2: ROXR/ROXL, 3: ROR/ROL
        target_reg = w0 & 7
        
        type_names = {0: "AS", 1: "LS", 2: "ROX", 3: "RO"}
        prefix = type_names[shift_type]
        dir_suffix = "L" if dr == 1 else "R"
        mnemonic = f"{prefix}{dir_suffix}"

        if ir == 0: # immediate shift
            count = 8 if count_or_reg == 0 else count_or_reg
            # Variable cycles in Musashi: count * 14 master cycles (+ 6 CPU cycles = 42 master cycles)
            return DecodedInstruction(pc, words, mnemonic, "w", [Operand("imm", val=count), Operand("reg_d", reg=target_reg)], base_cycles=42, variable_cycles=count * 14)
        else: # register shift
            return DecodedInstruction(pc, words, mnemonic, "w", [Operand("reg_d", reg=count_or_reg), Operand("reg_d", reg=target_reg)], base_cycles=42, variable_cycles=-1)

    raise NotImplementedError(f"Unsupported opcode 0x{w0:04X} at 0x{pc:06X}")
