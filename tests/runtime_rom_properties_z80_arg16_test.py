"""Guard the patched Z80 ARG16 operand reads, PC advance, and next-opcode alignment."""

import ast
from pathlib import Path
import sys


def arg16_replacement(patcher: Path) -> str:
    tree = ast.parse(patcher.read_text(encoding="utf-8"))
    for node in ast.walk(tree):
        if not isinstance(node, ast.Call) or len(node.args) < 3:
            continue
        if not isinstance(node.args[1], ast.Constant):
            continue
        if node.args[1].value == "  PC += 2;\n  return cpu_readop_arg(pc) | (cpu_readop_arg((pc+1)&0xffff) << 8);":
            replacement = node.args[2]
            if isinstance(replacement, ast.Constant) and isinstance(replacement.value, str):
                return replacement.value
    raise AssertionError("ARG16 rewrite is missing from the runtime patcher")


def main() -> None:
    replacement = arg16_replacement(Path(sys.argv[1]))
    assert replacement.count("cpu_readop_arg(pc)") == 1
    assert replacement.count("cpu_readop_arg((pc + 1u) & 0xffffu)") == 1
    assert replacement.count("oasis_romprops_z80_fetch(") == 2
    advance = replacement.index("PC += 2;")
    return_value = replacement.index("return low | ((UINT32)high << 8);")
    assert advance < return_value

    # LD HL,0x1234 followed by LD A,(HL): the second opcode must stay aligned.
    stream = [0x21, 0x34, 0x12, 0x7E]
    pc = 1
    low = stream[pc]
    high = stream[pc + 1]
    pc += 2
    assert low | high << 8 == 0x1234
    assert pc == 3
    assert stream[pc] == 0x7E


if __name__ == "__main__":
    main()
