# AI Development Contract

This document defines the mandatory operating discipline for AI-assisted development in Sega Thor.

## Core execution rules

1. Start implementation only when understanding is at least 90%. Before asking the user, first inspect repository documentation, current code, worklog, reverse-engineering notes, and available evidence.
2. If understanding is below 90% after inspection, stop implementation and ask only the minimum clarifying questions required.
3. Define explicit acceptance criteria before substantive implementation.
4. Do not declare a task DONE until implementation, build/tests, behavioral verification, documentation, and exact next step are all recorded.
5. If blocked, prove the blocker. Record what was checked, what evidence is missing, why work cannot continue safely, and the smallest next action.
6. Work on one active technical result at a time. Future ideas go to roadmap/backlog; do not switch milestones casually.
7. Do not expand scope, redesign architecture, or add speculative frameworks without a documented need and ADR when architectural.
8. Prefer evidence over elegance. Reverse-engineering statements must be marked CONFIRMED, LIKELY, HYPOTHESIS, or UNKNOWN.
9. Never turn a hypothesis into a fact without new evidence.
10. Never rename an unknown original routine to a confident semantic name without evidence.
11. Keep every source and project-document file at or below 500 lines. At roughly 400 lines, evaluate semantic splitting.
12. Do not change tests merely to make a failing implementation pass. Determine whether implementation, understanding, or test is wrong.
13. Prefer two independent verification methods for critical reverse-engineered behavior when practical.
14. Keep commits/tasks small and single-purpose.
15. Every meaningful task must update the project record: WORKLOG, REVERSE_ENGINEERING when applicable, DECISIONS when applicable, FILE_MAP when structure changes, ROADMAP/PROJECT_STATE when status changes.
16. Explanations to the user should be concise: what was found, what changed, how it was verified, what is next.
17. If a prior implementation is wrong, say so directly and record the reason and fix.
18. Do not optimize without measurement.
19. Do not work on The Story of Thor 2, Saturn support, widescreen, remaster features, or unrelated engine work until explicitly allowed by roadmap.
20. Faithful behavior comes first; enhancements come only after verified parity.

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
UNDERSTANDING CONFIDENCE:
ACCEPTANCE CRITERIA:
EVIDENCE AVAILABLE:
KNOWN UNKNOWNS:
```

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
