#!/usr/bin/env python3
"""
M68000 Instruction Decoder for Thor Recompiler.
Decodes instruction subset for tilemap pipelines, camera scroll, sound driver, and gameplay logic.
Strictly <= 500 lines per AGENTS.md Rule 3.
"""
from dataclasses import dataclass, field
import struct
from typing import List, Optional

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
    variable_cycles: int = 0
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

    # NOT: 0x4600 | reg (NOT.B Dn)
    if (w0 & 0xFFF8) == 0x4600:
        reg = w0 & 7
        return DecodedInstruction(pc, words, "NOT", "b", [Operand("reg_d", reg=reg, size="b")], base_cycles=4)

    # CLR: 0x4200 (bits 7..6 size, bits 5..3 mode, bits 2..0 reg)
    if (w0 & 0xFF00) == 0x4200:
        sz = "b" if (w0 & 0x00C0) == 0x0000 else ("w" if (w0 & 0x00C0) == 0x0040 else "l")
        mode = (w0 >> 3) & 7
        reg = w0 & 7
        if mode == 0:
            return DecodedInstruction(pc, words, "CLR", sz, [Operand("reg_d", reg=reg, size=sz)], base_cycles=4 if sz != "l" else 6)
        elif mode == 5:
            disp = struct.unpack_from(">h", rom, pc + 2)[0]
            words.append(disp & 0xFFFF)
            return DecodedInstruction(pc, words, "CLR", sz, [Operand("disp_a", reg=reg, val=disp, size=sz)], base_cycles=8 if sz != "l" else 12)
        elif mode == 7 and reg == 1:
            addr = struct.unpack_from(">I", rom, pc + 2)[0]
            words.extend([(addr >> 16) & 0xFFFF, addr & 0xFFFF])
            return DecodedInstruction(pc, words, "CLR", sz, [Operand("abs_l", val=addr, size=sz)], base_cycles=12 if sz != "l" else 16)

    # NEG: 0x4400 (0x4440 | reg for .w, mode 0)
    if (w0 & 0xFFF8) == 0x4440:
        reg = w0 & 7
        return DecodedInstruction(pc, words, "NEG", "w", [Operand("reg_d", reg=reg, size="w")], base_cycles=4)

    # TST: 0x4A00
    if (w0 & 0xFF00) == 0x4A00:
        sz = "b" if (w0 & 0x00C0) == 0x0000 else ("w" if (w0 & 0x00C0) == 0x0040 else "l")
        mode = (w0 >> 3) & 7
        reg = w0 & 7
        if mode == 0:
            return DecodedInstruction(pc, words, "TST", sz, [Operand("reg_d", reg=reg, size=sz)], base_cycles=4)
        elif mode == 5:
            disp = struct.unpack_from(">h", rom, pc + 2)[0]
            words.append(disp & 0xFFFF)
            return DecodedInstruction(pc, words, "TST", sz, [Operand("disp_a", reg=reg, val=disp, size=sz)], base_cycles=8)
        elif mode == 2:
            return DecodedInstruction(pc, words, "TST", sz, [Operand("indir_a", reg=reg, size=sz)], base_cycles=8)
        elif mode == 7 and reg == 1:
            addr = struct.unpack_from(">I", rom, pc + 2)[0]
            words.extend([(addr >> 16) & 0xFFFF, addr & 0xFFFF])
            return DecodedInstruction(pc, words, "TST", sz, [Operand("abs_l", val=addr, size=sz)], base_cycles=12)

    # Scc (SF.B / ST.B): 0x50C0..0x5FC0
    if (w0 & 0xF0C0) == 0x50C0:
        cond = (w0 >> 8) & 0x0F
        mnemonic = "ST" if cond == 0 else ("SF" if cond == 1 else f"S{cond}")
        mode = (w0 >> 3) & 7
        reg = w0 & 7
        if mode == 5:
            disp = struct.unpack_from(">h", rom, pc + 2)[0]
            words.append(disp & 0xFFFF)
            return DecodedInstruction(pc, words, mnemonic, "b", [Operand("disp_a", reg=reg, val=disp, size="b")], base_cycles=8)
        elif mode == 7 and reg == 1:
            addr = struct.unpack_from(">I", rom, pc + 2)[0]
            words.extend([(addr >> 16) & 0xFFFF, addr & 0xFFFF])
            return DecodedInstruction(pc, words, mnemonic, "b", [Operand("abs_l", val=addr, size="b")], base_cycles=12)

    # MOVEQ: 0x7000..0x7F00
    if (w0 & 0xF100) == 0x7000:
        reg = (w0 >> 9) & 7
        data = struct.unpack(">b", bytes([w0 & 0xFF]))[0]
        return DecodedInstruction(pc, words, "MOVEQ", "l", [Operand("imm", val=data, size="l"), Operand("reg_d", reg=reg, size="l")], base_cycles=4)

    # Bit manipulation with immediate bit number: BTST / BSET / BCLR / BCHG (0x0800..0x08FF)
    if (w0 & 0xFF00) == 0x0800:
        bit_op = (w0 >> 6) & 3
        op_names = {0: "BTST", 1: "BCHG", 2: "BCLR", 3: "BSET"}
        mnemonic = op_names[bit_op]
        bit_word = struct.unpack_from(">H", rom, pc + 2)[0]
        words.append(bit_word)
        bit_num = bit_word & 0xFF
        dst_mode = (w0 >> 3) & 7
        dst_reg = w0 & 7
        if dst_mode == 0:
            return DecodedInstruction(pc, words, mnemonic, "l", [Operand("imm", val=bit_num), Operand("reg_d", reg=dst_reg, size="l")], base_cycles=12)
        elif dst_mode == 5:
            disp = struct.unpack_from(">h", rom, pc + 4)[0]
            words.append(disp & 0xFFFF)
            return DecodedInstruction(pc, words, mnemonic, "b", [Operand("imm", val=bit_num), Operand("disp_a", reg=dst_reg, val=disp, size="b")], base_cycles=12)
        elif dst_mode == 7 and dst_reg == 1:
            addr = struct.unpack_from(">I", rom, pc + 4)[0]
            words.extend([(addr >> 16) & 0xFFFF, addr & 0xFFFF])
            return DecodedInstruction(pc, words, mnemonic, "b", [Operand("imm", val=bit_num), Operand("abs_l", val=addr, size="b")], base_cycles=16)

    # Dynamic bit manipulation with register: BTST / BSET / BCLR / BCHG (0x0100..0x01FF)
    if (w0 & 0xF100) in (0x0100, 0x0140, 0x0180, 0x01C0):
        bit_op = (w0 >> 6) & 3
        mnemonic = {0: "BTST", 1: "BCHG", 2: "BCLR", 3: "BSET"}[bit_op]
        d_reg = (w0 >> 9) & 7
        dst_mode = (w0 >> 3) & 7
        dst_reg = w0 & 7
        if dst_mode == 7 and dst_reg == 1:
            addr = struct.unpack_from(">I", rom, pc + 2)[0]
            words.extend([(addr >> 16) & 0xFFFF, addr & 0xFFFF])
            return DecodedInstruction(pc, words, mnemonic, "b", [Operand("reg_d", reg=d_reg, size="b"), Operand("abs_l", val=addr, size="b")], base_cycles=12)
        elif dst_mode == 0:
            return DecodedInstruction(pc, words, mnemonic, "l", [Operand("reg_d", reg=d_reg, size="l"), Operand("reg_d", reg=dst_reg, size="l")], base_cycles=8)

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
            sz = "w"
        else:
            target = pc + 2 + disp8
            sz = "b"
        return DecodedInstruction(pc, words, mnemonic, sz, [Operand("branch_disp", val=target)], branch_target=target, base_cycles=10)

    # DBcc / DBRA: 0x50C8..0x5FCF
    if (w0 & 0xF0F8) == 0x50C8:
        reg = w0 & 7
        cond = (w0 >> 8) & 0x0F
        mnemonic = "DBRA" if cond == 1 else f"DB{cond}"
        w1 = struct.unpack_from(">h", rom, pc + 2)[0]
        words.append(w1 & 0xFFFF)
        target = pc + 2 + w1
        return DecodedInstruction(pc, words, mnemonic, "w", [Operand("reg_d", reg=reg), Operand("branch_disp", val=target)], branch_target=target, base_cycles=10)

    # ADDQ / SUBQ: 0x5000 / 0x5100
    if (w0 & 0xF000) == 0x5000 and (w0 & 0x00C0) != 0x00C0:
        is_sub = (w0 & 0x0100) != 0
        data = (w0 >> 9) & 7
        imm = 8 if data == 0 else data
        sz = "b" if (w0 & 0x00C0) == 0 else ("w" if (w0 & 0x00C0) == 0x0040 else "l")
        mode = (w0 >> 3) & 7
        reg = w0 & 7
        mnemonic = "SUBQ" if is_sub else "ADDQ"
        if mode == 0:
            return DecodedInstruction(pc, words, mnemonic, sz, [Operand("imm", val=imm), Operand("reg_d", reg=reg, size=sz)], base_cycles=4 if sz != "l" else 8)
        elif mode == 1:
            return DecodedInstruction(pc, words, mnemonic, sz, [Operand("imm", val=imm), Operand("reg_a", reg=reg, size=sz)], base_cycles=8)
        elif mode == 5:
            disp = struct.unpack_from(">h", rom, pc + 2)[0]
            words.append(disp & 0xFFFF)
            return DecodedInstruction(pc, words, mnemonic, sz, [Operand("imm", val=imm), Operand("disp_a", reg=reg, val=disp, size=sz)], base_cycles=12)

    # LEA: 0x41C0..0x4FF8
    if (w0 & 0xF1C0) == 0x41C0:
        a_reg = (w0 >> 9) & 7
        mode = (w0 >> 3) & 7
        reg = w0 & 7
        if mode == 7 and reg == 1:  # abs.l
            addr = struct.unpack_from(">I", rom, pc + 2)[0]
            words.extend([(addr >> 16) & 0xFFFF, addr & 0xFFFF])
            return DecodedInstruction(pc, words, "LEA", "l", [Operand("abs_l", val=addr), Operand("reg_a", reg=a_reg)], base_cycles=12)
        elif mode == 7 and reg == 2:  # d16(PC)
            disp = struct.unpack_from(">h", rom, pc + 2)[0]
            words.append(disp & 0xFFFF)
            target_addr = pc + 2 + disp
            return DecodedInstruction(pc, words, "LEA", "l", [Operand("imm", val=target_addr, size="l"), Operand("reg_a", reg=a_reg)], base_cycles=8)
        elif mode == 7 and reg == 3:  # d8(PC, Xn)
            ext = struct.unpack_from(">H", rom, pc + 2)[0]
            words.append(ext)
            x_reg = (ext >> 12) & 7
            disp = struct.unpack(">b", bytes([ext & 0xFF]))[0]
            base_addr = pc + 2 + disp
            return DecodedInstruction(pc, words, "LEA", "l", [Operand("pc_index", reg=x_reg, val=base_addr, size="w"), Operand("reg_a", reg=a_reg)], base_cycles=12)
        elif mode == 5:  # d16(An)
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
        elif src_mode == 7 and src_reg == 1:
            addr = struct.unpack_from(">I", rom, pc + offset)[0]
            words.extend([(addr >> 16) & 0xFFFF, addr & 0xFFFF])
            offset += 4
            src_op = Operand("abs_l", val=addr, size=sz)
        elif src_mode == 7 and src_reg == 4:
            if sz == "l":
                val = struct.unpack_from(">I", rom, pc + offset)[0]
                words.extend([(val >> 16) & 0xFFFF, val & 0xFFFF])
                offset += 4
            else:
                val = struct.unpack_from(">H", rom, pc + offset)[0]
                if sz == "b": val &= 0xFF
                words.append(val)
                offset += 2
            src_op = Operand("imm", val=val, size=sz)
        else:
            raise NotImplementedError(f"Unsupported MOVE src mode={src_mode} reg={src_reg} at 0x{pc:06X}")

        if dst_mode == 0:
            dst_op = Operand("reg_d", reg=dst_reg, size=sz)
        elif dst_mode == 1:
            return DecodedInstruction(pc, words, "MOVEA", sz, [src_op, Operand("reg_a", reg=dst_reg, size=sz)], base_cycles=8 if sz=="w" else 12)
        elif dst_mode == 2:
            dst_op = Operand("indir_a", reg=dst_reg, size=sz)
        elif dst_mode == 3:
            dst_op = Operand("postinc_a", reg=dst_reg, size=sz)
        elif dst_mode == 5:
            disp = struct.unpack_from(">h", rom, pc + offset)[0]
            words.append(disp & 0xFFFF)
            dst_op = Operand("disp_a", reg=dst_reg, val=disp, size=sz)
        elif dst_mode == 7 and dst_reg == 1:
            addr = struct.unpack_from(">I", rom, pc + offset)[0]
            words.extend([(addr >> 16) & 0xFFFF, addr & 0xFFFF])
            dst_op = Operand("abs_l", val=addr, size=sz)
        else:
            raise NotImplementedError(f"Unsupported MOVE dst mode={dst_mode} reg={dst_reg} at 0x{pc:06X}")

        return DecodedInstruction(pc, words, "MOVE", sz, [src_op, dst_op], base_cycles=8 if sz!="l" else 12)

    # Immediates: ADDI, SUBI, CMPI, ANDI, ORI, EORI (0x0000..0x0E00)
    imm_op_types = {0x00: "ORI", 0x02: "ANDI", 0x04: "SUBI", 0x06: "ADDI", 0x0A: "EORI", 0x0C: "CMPI"}
    type_byte = (w0 >> 8) & 0xFF
    if type_byte in imm_op_types and (w0 & 0x00C0) in (0x0000, 0x0040, 0x0080):
        sz = "b" if (w0 & 0x00C0) == 0x0000 else ("w" if (w0 & 0x00C0) == 0x0040 else "l")
        mnemonic = imm_op_types[type_byte]
        if sz == "b":
            imm = struct.unpack_from(">H", rom, pc + 2)[0] & 0xFF
            words.append(imm)
            offset = 4
        elif sz == "w":
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

    # OR: 0x8000
    if (w0 & 0xF000) == 0x8000:
        reg = (w0 >> 9) & 7
        opmode = (w0 >> 6) & 7
        src_mode = (w0 >> 3) & 7
        src_reg = w0 & 7
        if opmode == 0:  # OR.B <ea>, Dn
            if src_mode == 0:
                return DecodedInstruction(pc, words, "OR", "b", [Operand("reg_d", reg=src_reg, size="b"), Operand("reg_d", reg=reg, size="b")], base_cycles=4)
            elif src_mode == 5:
                disp = struct.unpack_from(">h", rom, pc + 2)[0]
                words.append(disp & 0xFFFF)
                return DecodedInstruction(pc, words, "OR", "b", [Operand("disp_a", reg=src_reg, val=disp, size="b"), Operand("reg_d", reg=reg, size="b")], base_cycles=8)

    # ADD / SUB / ADDA / SUBA
    if (w0 & 0xF000) in (0xD000, 0x9000):
        is_sub = (w0 & 0xF000) == 0x9000
        reg = (w0 >> 9) & 7
        opmode = (w0 >> 6) & 7
        src_mode = (w0 >> 3) & 7
        src_reg = w0 & 7

        if opmode in (3, 7):  # ADDA / SUBA
            sz = "w" if opmode == 3 else "l"
            mnemonic = "SUBA" if is_sub else "ADDA"
            if src_mode == 0:
                return DecodedInstruction(pc, words, mnemonic, sz, [Operand("reg_d", reg=src_reg, size=sz), Operand("reg_a", reg=reg, size="l")], base_cycles=8)
            elif src_mode == 1:
                return DecodedInstruction(pc, words, mnemonic, sz, [Operand("reg_a", reg=src_reg, size=sz), Operand("reg_a", reg=reg, size="l")], base_cycles=8)
            elif src_mode == 2:
                return DecodedInstruction(pc, words, mnemonic, sz, [Operand("indir_a", reg=src_reg, size=sz), Operand("reg_a", reg=reg, size="l")], base_cycles=8)
        elif opmode == 0:  # ADD.B / SUB.B (<ea>, Dn)
            mnemonic = "SUB" if is_sub else "ADD"
            if src_mode == 0:
                return DecodedInstruction(pc, words, mnemonic, "b", [Operand("reg_d", reg=src_reg, size="b"), Operand("reg_d", reg=reg, size="b")], base_cycles=4)
            elif src_mode == 5:
                disp = struct.unpack_from(">h", rom, pc + 2)[0]
                words.append(disp & 0xFFFF)
                return DecodedInstruction(pc, words, mnemonic, "b", [Operand("disp_a", reg=src_reg, val=disp, size="b"), Operand("reg_d", reg=reg, size="b")], base_cycles=8)
        elif opmode == 4:  # ADD.B / SUB.B (Dn, <ea>)
            mnemonic = "SUB" if is_sub else "ADD"
            if src_mode == 5:
                disp = struct.unpack_from(">h", rom, pc + 2)[0]
                words.append(disp & 0xFFFF)
                return DecodedInstruction(pc, words, mnemonic, "b", [Operand("reg_d", reg=reg, size="b"), Operand("disp_a", reg=src_reg, val=disp, size="b")], base_cycles=8)
        elif opmode == 2:  # ADD.L / SUB.L (<ea>, Dn)
            mnemonic = "SUB" if is_sub else "ADD"
            if src_mode == 0:
                return DecodedInstruction(pc, words, mnemonic, "l", [Operand("reg_d", reg=src_reg, size="l"), Operand("reg_d", reg=reg, size="l")], base_cycles=6)
            elif src_mode == 5:
                disp = struct.unpack_from(">h", rom, pc + 2)[0]
                words.append(disp & 0xFFFF)
                return DecodedInstruction(pc, words, mnemonic, "l", [Operand("disp_a", reg=src_reg, val=disp, size="l"), Operand("reg_d", reg=reg, size="l")], base_cycles=12)
        elif opmode == 5:  # ADD.W / SUB.W (Dn, <ea>)
            mnemonic = "SUB" if is_sub else "ADD"
            if src_mode == 5:
                disp = struct.unpack_from(">h", rom, pc + 2)[0]
                words.append(disp & 0xFFFF)
                return DecodedInstruction(pc, words, mnemonic, "w", [Operand("reg_d", reg=reg, size="w"), Operand("disp_a", reg=src_reg, val=disp, size="w")], base_cycles=12)
        elif opmode == 6:  # ADD.L / SUB.L (Dn, <ea>)
            mnemonic = "SUB" if is_sub else "ADD"
            if src_mode == 5:
                disp = struct.unpack_from(">h", rom, pc + 2)[0]
                words.append(disp & 0xFFFF)
                return DecodedInstruction(pc, words, mnemonic, "l", [Operand("reg_d", reg=reg, size="l"), Operand("disp_a", reg=src_reg, val=disp, size="l")], base_cycles=16)
        elif opmode == 1:  # ADD.W / SUB.W (<ea>, Dn)
            mnemonic = "SUB" if is_sub else "ADD"
            if src_mode == 0:
                return DecodedInstruction(pc, words, mnemonic, "w", [Operand("reg_d", reg=src_reg), Operand("reg_d", reg=reg)], base_cycles=4)
            elif src_mode == 5:
                disp = struct.unpack_from(">h", rom, pc + 2)[0]
                words.append(disp & 0xFFFF)
                return DecodedInstruction(pc, words, mnemonic, "w", [Operand("disp_a", reg=src_reg, val=disp), Operand("reg_d", reg=reg)], base_cycles=8)

    # CMP / CMPA: 0xB000
    if (w0 & 0xF000) == 0xB000:
        opmode = (w0 >> 6) & 7
        src_mode = (w0 >> 3) & 7
        src_reg = w0 & 7
        if opmode in (3, 7):  # CMPA
            sz = "w" if opmode == 3 else "l"
            a_dst = (w0 >> 9) & 7
            if src_mode == 7 and src_reg == 4:  # imm
                if sz == "l":
                    imm = struct.unpack_from(">I", rom, pc + 2)[0]
                    words.extend([(imm >> 16) & 0xFFFF, imm & 0xFFFF])
                else:
                    imm = struct.unpack_from(">H", rom, pc + 2)[0]
                    words.append(imm)
                return DecodedInstruction(pc, words, "CMPA", sz, [Operand("imm", val=imm, size=sz), Operand("reg_a", reg=a_dst, size="l")], base_cycles=6)
        elif opmode == 1:
            d_dst = (w0 >> 9) & 7
            if src_mode == 0:
                return DecodedInstruction(pc, words, "CMP", "w", [Operand("reg_d", reg=src_reg), Operand("reg_d", reg=d_dst)], base_cycles=4)
            elif src_mode == 5:
                disp = struct.unpack_from(">h", rom, pc + 2)[0]
                words.append(disp & 0xFFFF)
                return DecodedInstruction(pc, words, "CMP", "w", [Operand("disp_a", reg=src_reg, val=disp), Operand("reg_d", reg=d_dst)], base_cycles=8)

    # JSR: 0x4EB9
    if w0 == 0x4EB9:
        target = struct.unpack_from(">I", rom, pc + 2)[0]
        words.extend([(target >> 16) & 0xFFFF, target & 0xFFFF])
        return DecodedInstruction(pc, words, "JSR", "l", [Operand("abs_l", val=target, size="l")], branch_target=target, base_cycles=16)

    # JMP: 0x4EC0..0x4EFF
    if (w0 & 0xFFC0) == 0x4EC0:
        mode = (w0 >> 3) & 7
        reg = w0 & 7
        if mode == 2:
            return DecodedInstruction(pc, words, "JMP", "", [Operand("indir_a", reg=reg)], base_cycles=8)

    # MOVEM: 0x48E7 (-(A7)), 0x4CDF ((A7)+)
    if w0 in (0x48E7, 0x4CDF):
        is_pop = (w0 == 0x4CDF)
        mask = struct.unpack_from(">H", rom, pc + 2)[0]
        words.append(mask)
        mnemonic = "MOVEM"
        mode_str = "postinc_a" if is_pop else "predec_a"
        cycles = 12 + (bin(mask).count("1") * 8)
        if is_pop:
            return DecodedInstruction(pc, words, mnemonic, "l", [Operand(mode_str, reg=7), Operand("imm", val=mask, size="w")], base_cycles=cycles)
        else:
            return DecodedInstruction(pc, words, mnemonic, "l", [Operand("imm", val=mask, size="w"), Operand(mode_str, reg=7)], base_cycles=cycles)

    # MULS / MULU: 0xC1C0 / 0xC0C0
    if (w0 & 0xF0C0) in (0xC1C0, 0xC0C0):
        is_signed = (w0 & 0x0100) != 0
        reg = (w0 >> 9) & 7
        src_mode = (w0 >> 3) & 7
        src_reg = w0 & 7
        mnemonic = "MULS" if is_signed else "MULU"
        if src_mode == 0:
            return DecodedInstruction(pc, words, mnemonic, "w", [Operand("reg_d", reg=src_reg, size="w"), Operand("reg_d", reg=reg, size="w")], base_cycles=54)

    # DIVS / DIVU: 0x81C0 / 0x80C0
    if (w0 & 0xF0C0) in (0x81C0, 0x80C0):
        is_signed = (w0 & 0x0100) != 0
        reg = (w0 >> 9) & 7
        src_mode = (w0 >> 3) & 7
        src_reg = w0 & 7
        mnemonic = "DIVS" if is_signed else "DIVU"
        if src_mode == 0:
            return DecodedInstruction(pc, words, mnemonic, "w", [Operand("reg_d", reg=src_reg, size="w"), Operand("reg_d", reg=reg, size="w")], base_cycles=140)

    # Shifts / Rotates: 0xE000
    if (w0 & 0xF000) == 0xE000:
        count_or_reg = (w0 >> 9) & 7
        dr = (w0 >> 8) & 1
        ir = (w0 >> 5) & 1
        shift_type = (w0 >> 3) & 3
        target_reg = w0 & 7

        type_names = {0: "AS", 1: "LS", 2: "ROX", 3: "RO"}
        prefix = type_names[shift_type]
        dir_suffix = "L" if dr == 1 else "R"
        mnemonic = f"{prefix}{dir_suffix}"

        if ir == 0:
            count = 8 if count_or_reg == 0 else count_or_reg
            return DecodedInstruction(pc, words, mnemonic, "w", [Operand("imm", val=count), Operand("reg_d", reg=target_reg)], base_cycles=42, variable_cycles=count * 14)
        else:
            return DecodedInstruction(pc, words, mnemonic, "w", [Operand("reg_d", reg=count_or_reg), Operand("reg_d", reg=target_reg)], base_cycles=42, variable_cycles=-1)

    raise NotImplementedError(f"Unsupported opcode 0x{w0:04X} at 0x{pc:06X}")
