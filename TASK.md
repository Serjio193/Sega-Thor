# Current Task

TASK: M11.5 — Second bounded 68000 RE acceleration slice
WHY: The accepted `0x60004` checkpoint must scale to several evidenced
routines without becoming an emulator, recompiler or gameplay dependency.
CURRENT MILESTONE: M11.5
MILESTONE UNDERSTANDING CONFIDENCE: 95%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 95%
SLICE MODE: RE_TOOLING_ONLY
STATUS: COMPLETE

## Scope

This is a bounded tooling checkpoint, not a gameplay/runtime milestone. The
tool is research infrastructure only and must not become part of the native
game runtime.

Representative targets: exact-boundary `0x3820` and `0xD3B2`; bounded-only
`0x8E90` and `0xA6A4`. All are selected from existing RE evidence and retain
raw addresses only.

## Acceptance criteria

- [x] use the user-supplied canonical USA ROM only; do not commit ROM bytes or extracted commercial assets;
- [x] analyze several existing evidenced routines with different control-flow forms;
- [x] discover a return boundary only when all bounded paths support that conclusion;
- [x] report caller/callee and block/instruction direct-call edges separately from indirect/unresolved flow;
- [x] bind confirmed, unresolved and unsupported memory evidence to function, slice, block and instruction;
- [x] emit deterministic `oasis.m68k.re-program.v1` JSON and a short human report;
- [x] add synthetic tests, a USA-ROM oracle and automatic comparisons with existing RE evidence;
- [x] update FILE_MAP / WORKLOG / REVERSE_ENGINEERING / PROJECT_STATE as required;
- [x] keep every source/document file <= 500 lines;
- [x] Debug/Release builds and CTest are green.

## Result

The multi-slice report analyzes 4 functions, 421 instructions and 131 basic
blocks. It records 1 direct call site and 1 caller→callee edge, 18 confirmed
memory references, 114 unresolved register-based references, 1 indirect jump,
and 2 unsupported opcodes. Known boundaries for `0x3820` and `0xD3B2` remain
confirmed; `0x8E90` and `0xA6A4` remain bounded-only. USA evidence reproduces
the documented decompressor/reader call, pool dispatch edges, callback jump,
table and RAM destination references. No production gameplay behavior was added.

## Hard boundaries

- Do not build a full Motorola 68000 emulator.
- Do not build a general-purpose Mega Drive emulator.
- Do not attempt whole-ROM recompilation in this checkpoint.
- Do not add generated 68000 instruction-by-instruction code to the native gameplay runtime.
- Do not invent semantic names for unknown routines, RAM fields, driver commands or event meanings.
- Do not start M12 implementation.
- Do not add a broad dependency/framework unless a narrow experiment proves it is required.

## Known unknowns

Driver command meanings, audio protocol, event/progression semantics and the
producer caller remain UNKNOWN. The decoder is intentionally bounded; the
aggregate is not a generic 68000 decoder, emulator, whole-ROM discovery pass or
recompiler. Register-based references and indirect targets remain unresolved.

## Exact next action

Stop at this verified checkpoint and await explicit instruction before dynamic
tracing, similarity search, whole-ROM discovery, recompilation or M12 work.
