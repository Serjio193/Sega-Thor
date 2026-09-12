# M12 A372 runtime register/root capture — bounded negative natural scenario

This older report covers the 1,800-frame natural scenario and remains a
bounded negative result for that scenario. It is superseded as the next-step
decision by the later controlled QuickSave1 root capture in
`THOR_M12_CONTROLLED_RUNTIME_ROOT_CAPTURE.md`; the restored harness does not
make the natural-scenario target reachable and does not establish Right
causality.

Baseline: `origin/main = 67fec9592a9476a8a28d685a4dca6685c970d990`.

Canonical ROM: `0x300000` bytes, SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.

`SOURCE_OWNED` remains `1,475,368 / 3,145,728 = 46.9006856283%`.
No ROM bytes, assets, savestates, decoded payload, or production-runtime code
were added.

## Selected fork

The previous caller-join report ranked a targeted runtime capture at
`A196 -> A342 -> A372` highest because static analysis could not expose the
runtime values of `FF1858`, `FF188C`, `D2`, `D5`, and `A5`. The probe is
developer-only and records bounded register/RAM context and source-write
addresses only; it does not read ROM payload or emit emulator state writes.

## Pass 1 — static/preflight

The existing canonical ROM identity and static contracts were revalidated:
`0x008B86 -> 0x00A196`, `0x00A19C -> 0x00A342`, and `0x00A372` as the
`MOVE.L D2,(A5)+` source producer. The new Lua probe, Python summarizer,
synthetic regression, and Python compilation passed. The expected static join
remains:

`A196 -> A342 -> A372 -> FF13CC -> DMA 0x27EC -> SAT VRAM 0xD000`.

## Pass 2 — bounded runtime replay

BizHawk `2.11.1` was run with the canonical ROM, the existing
`m11_8_natural_reachability_v1` hardware-reset scenario, and exactly 1,800
frames. The probe exited `0`; `state_writes_emitted=false`.

| Observation | Result |
| --- | --- |
| `A196` execution hooks | `0` |
| `A342` execution hooks | `0` |
| `A372` execution hooks | `0` |
| runtime root observations | none |
| bounded source-write records | `4096`, cap reached; not interpreted |
| register/root samples | none |

The ignored capture is
`build/m12-gfx-runtime/a372-runtime-pass2.json`, 312,460 bytes, SHA-256
`eeb8489191a3649a32237b701ced7b294d0f4edcab842a6f8019398e0af562e2`.
The zero hook result is a bounded scenario result only. It does not disprove
the statically proven scheduler edge or prove that `A196` is unreachable in a
different controlled state. The saturated source-write list is discarded as
non-diagnostic because it lacks a paired A372 hook.

## Evidence boundary

This session adds no runtime root-selection, register-lifetime, same-frame SAT,
object, animation, frame, or sprite-piece claim. The strongest supported graph
therefore remains the static join from the previous report, while the natural
scenario is now classified as `NO_NEW_TARGET_RUNTIME_REACH` for this bounded
target set.

No `SOURCE_OWNED` promotion is justified. The developer-only probe is
`src/tools/re_bizhawk_m12_a372_runtime.lua`; its payload-free validator is
`src/tools/m12_a372_runtime_report.py`, with regression coverage in
`tests/m12_a372_runtime_report_test.py`.

## Next directions

1. Use a separately validated controlled BizHawk state, if available, to reach
   the A372 path; do not manufacture or force RAM/register state in this
   session.
2. Close the static `FF1858` writer/consumer family outside the already
   classified ranges.
3. Continue the independent `0x03BDA6` consumer-boundary investigation.

AUTONOMOUS SESSION STOP REASON: the highest-information runtime fork produced
no target execution in the frozen 1,800-frame scenario. The two-pass rule is
complete, replay expansion is not repeated, and further progress requires a
new controlled-state evidence source rather than another equivalent natural
replay.
