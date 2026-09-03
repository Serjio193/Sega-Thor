# Current Task

TASK: M11.5 — Narrow 68000 RE acceleration experiment
WHY: M11 is complete. Before starting M12, test whether a small static 68000 analysis tool can materially reduce manual reverse-engineering work on already-evidenced routine `0x60004`.
CURRENT MILESTONE: M11.5
MILESTONE UNDERSTANDING CONFIDENCE: 95%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 95%
SLICE MODE: RE_TOOLING_ONLY
STATUS: ACTIVE

## Scope
This is a bounded tooling checkpoint, not a gameplay/runtime milestone. The tool is research infrastructure only and must not become part of the native game runtime.

Initial target: USA-ROM routine `0x00060004`, already encountered and partially bounded during M11.

## Acceptance criteria
- [ ] use the user-supplied canonical USA ROM only; do not commit ROM bytes or extracted commercial assets;
- [ ] add one small isolated RE tool that starts from `0x60004` and decodes enough Motorola 68000 control flow to produce useful evidence;
- [ ] identify basic blocks and direct branch/call targets reachable inside the bounded analysis;
- [ ] report direct ROM/RAM absolute references and immediate constants encountered where decoding is reliable;
- [ ] emit a deterministic human-readable report and a machine-readable JSON report;
- [ ] add synthetic tests for decoder/control-flow behavior and a local USA-ROM oracle for the `0x60004` experiment;
- [ ] verify that the generated result independently recovers already-known M11 evidence around the `0x60004` path;
- [ ] update FILE_MAP / WORKLOG / REVERSE_ENGINEERING as required by added files or new evidence;
- [ ] keep every source/document file <= 500 lines;
- [ ] CI green.

## Hard boundaries
- Do not build a full Motorola 68000 emulator.
- Do not build a general-purpose Mega Drive emulator.
- Do not attempt whole-ROM recompilation in this checkpoint.
- Do not add generated 68000 instruction-by-instruction code to the native gameplay runtime.
- Do not invent semantic names for unknown routines, RAM fields, driver commands or event meanings.
- Do not start M12 implementation while this checkpoint is ACTIVE.
- Do not add a broad dependency/framework unless the narrow `0x60004` experiment proves it is required.

## Evaluation gate
The experiment succeeds only if it makes the `0x60004` analysis materially easier than the current manual method. Merely producing a disassembly listing is insufficient.

Useful output should include, when recoverable:
- entry/basic-block addresses;
- direct branch and call edges;
- return/termination boundaries;
- absolute memory references;
- immediate constants;
- unresolved indirect targets explicitly marked UNKNOWN.

If the PoC cannot independently reproduce known M11 control-flow/data evidence or adds excessive complexity, stop it and return to the normal evidence-first RE workflow instead of expanding the tool.

## Exact next action
Implement the smallest `0x60004` static-analysis PoC, verify it against existing M11 evidence, then stop for review before adding dynamic tracing, similarity search, whole-ROM discovery or recompilation.
