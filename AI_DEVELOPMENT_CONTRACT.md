# AI Development Contract

This document defines the mandatory operating discipline for AI-assisted development in Sega Thor.

## Core execution rules

1. Track two confidence levels separately when useful: **milestone understanding** and **current implementation-slice understanding**. Milestone understanding may be below 90% during reverse engineering; this is expected for partially reconstructed systems.
2. Start or extend production C++ for a specific implementation slice only when **CURRENT SLICE UNDERSTANDING CONFIDENCE is at least 90%**. Confidence must be based on concrete evidence for that slice: callers/data, observed behavior, tests, ROM bytes/traces, or equivalent reproducible evidence.
3. If current-slice confidence is below 90% after inspecting repository documentation, code, worklog, reverse-engineering notes and available evidence, do **not** implement speculative production behavior. Continue reverse engineering, evidence gathering, documentation, probes or the minimum necessary clarification until the slice reaches at least 90%, becomes objectively BLOCKED, or requires a USER DECISION.
4. A low overall milestone confidence does not block a well-bounded >=90% slice. Conversely, high milestone confidence never authorizes a <90% slice.
5. Define explicit acceptance criteria before substantive implementation.
6. Do not declare a task DONE until implementation, build/tests, behavioral verification, documentation, and exact next step are all recorded.
7. If blocked, prove the blocker. Record what was checked, what evidence is missing, why work cannot continue safely, and the smallest next action.
8. Work on one active technical result at a time. Future ideas go to roadmap/backlog; do not switch milestones casually.
9. Do not expand scope, redesign architecture, or add speculative frameworks without a documented need and ADR when architectural.
10. Prefer evidence over elegance. Reverse-engineering statements must be marked CONFIRMED, LIKELY, HYPOTHESIS, or UNKNOWN.
11. Never turn a hypothesis into a fact without new evidence.
12. Never rename an unknown original routine to a confident semantic name without evidence.
13. Keep executable/source/build-code files at or below 500 lines. This includes C/C++ source and headers, tests, developer tooling and build scripts. Prose documentation, project instructions, worklogs, ADRs, task/state files and other reference Markdown are exempt from the numeric limit; split large documentation only when structure or navigation materially improves.
14. Do not change tests merely to make a failing implementation pass. Determine whether implementation, understanding, or test is wrong.
15. Prefer two independent verification methods for critical reverse-engineered behavior when practical.
16. Keep commits/tasks small and single-purpose.
17. Every meaningful task must update the project record: WORKLOG, REVERSE_ENGINEERING when applicable, DECISIONS when applicable, FILE_MAP when structure changes, ROADMAP/PROJECT_STATE when status changes.
18. Explanations to the user should be concise: what was found, what changed, how it was verified, what is next.
19. If a prior implementation is wrong, say so directly and record the reason and fix.
20. Do not optimize without measurement.
21. Do not work on The Story of Thor 2, Saturn support, widescreen, remaster features, or unrelated engine work until explicitly allowed by roadmap.
22. Faithful behavior comes first; enhancements come only after verified parity.

## Confidence interpretation

Use confidence narrowly and operationally:

```text
MILESTONE UNDERSTANDING CONFIDENCE:
CURRENT SLICE UNDERSTANDING CONFIDENCE:
```

Examples:

```text
MILESTONE UNDERSTANDING CONFIDENCE: 55%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 96%
```

This is valid: the full subsystem is still poorly understood, but one bounded routine/data contract is sufficiently evidenced to translate.

```text
MILESTONE UNDERSTANDING CONFIDENCE: 90%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 75%
```

This does **not** authorize production implementation of that slice. Continue RE/evidence work first.

Confidence is not a subjective optimism score. State briefly what evidence justifies >=90% when production implementation begins.

## Required task lifecycle

Every substantive task follows:

```text
UNKNOWN
  -> EVIDENCE
  -> UNDERSTOOD
  -> IMPLEMENTED
  -> VERIFIED
  -> DOCUMENTED
```

A step is not complete before DOCUMENTED.

## Required task header

Before code changes, record:

```text
TASK:
WHY:
CURRENT MILESTONE:
MILESTONE UNDERSTANDING CONFIDENCE:
CURRENT SLICE UNDERSTANDING CONFIDENCE:
SLICE CONFIDENCE EVIDENCE:
ACCEPTANCE CRITERIA:
EVIDENCE AVAILABLE:
KNOWN UNKNOWNS:
```

If `CURRENT SLICE UNDERSTANDING CONFIDENCE < 90%`, the task is in evidence-gathering mode and production C++ changes for that slice are prohibited.

## Allowed stop states

Work on the active task may stop only as:

### DONE
All acceptance criteria are met with recorded verification.

### BLOCKED
Continuation is objectively impossible without missing evidence/input, and the blocker is documented.

### USER DECISION REQUIRED
Multiple materially different valid choices remain and repository goals/evidence do not determine the choice.

Otherwise continue the current step.

## Session checkpoint

At the end of every work session record:

```text
CURRENT MILESTONE:
CURRENT TASK:
TASK STATUS:
MILESTONE UNDERSTANDING CONFIDENCE:
CURRENT SLICE UNDERSTANDING CONFIDENCE:
LAST VERIFIED RESULT:
FILES CHANGED:
TESTS RUN:
NEW KNOWLEDGE:
OPEN QUESTIONS:
BLOCKERS:
EXACT NEXT ACTION:
```

## Primary project question

Before any change ask:

> What exactly does this change prove, reproduce, or make verifiable about the original game?

If the answer is unclear, do not make the change.
