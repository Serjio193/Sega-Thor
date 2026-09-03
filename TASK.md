# Current Task

TASK: M11.5 — Narrow 68000 RE acceleration experiment
WHY: M11 is complete. Test whether a small static 68000 analysis tool materially reduces manual reverse-engineering work on evidenced routine `0x60004`.
CURRENT MILESTONE: M11.5
MILESTONE UNDERSTANDING CONFIDENCE: 95%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 95%
SLICE MODE: RE_TOOLING_ONLY
STATUS: COMPLETE

## Scope

This is a bounded tooling checkpoint, not a gameplay/runtime milestone. The
tool is research infrastructure only and must not become part of the native
game runtime.

Initial target: USA-ROM routine `0x00060004`, already encountered and
partially bounded during M11.

## Acceptance criteria

- [x] use the user-supplied canonical USA ROM only; do not commit ROM bytes or extracted commercial assets;
- [x] add one small isolated RE tool that starts from `0x60004` and decodes enough Motorola 68000 control flow to produce useful evidence;
- [x] identify basic blocks and direct branch/call targets reachable inside the bounded analysis;
- [x] report direct ROM/RAM absolute references and immediate constants encountered where decoding is reliable;
- [x] emit a deterministic human-readable report and a machine-readable JSON report;
- [x] add synthetic tests for decoder/control-flow behavior and a local USA-ROM oracle for the `0x60004` experiment;
- [x] verify that the generated result independently recovers already-known M11 control-flow/data evidence;
- [x] update FILE_MAP / WORKLOG / REVERSE_ENGINEERING as required;
- [x] keep every source/document file <= 500 lines;
- [x] Debug/Release builds and CTest are green.

## Result

The bounded report covers `[0x60004, 0x61204)`: 801 reachable instructions,
109 basic blocks, 72 direct branches, 17 direct calls, 3 absolute ROM
references and 114 absolute RAM references. It independently recovers
`0x60004 -> 0x6042A`, `0x60478 -> 0x609C6` and `0x60488 -> 0x60D10`.
The current reachable slice has no indirect or unsupported opcode; synthetic
coverage proves those categories are reported separately. JSON is deterministic
across Debug/Release. No production gameplay behavior was added.

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
producer caller remain UNKNOWN. The decoder is intentionally bounded and is
not a generic 68000 decoder or emulator.

## Exact next action

Stop at this verified checkpoint and await explicit instruction before dynamic
tracing, similarity search, whole-ROM discovery, recompilation or M12 work.
