# Development Rules

## Source limits
- Hard limit: **500 lines per file** for executable/source-code files, including C/C++ source and headers, tests, developer tooling, and build scripts such as CMake.
- Documentation, project instructions, worklogs, ADRs, task/state files, and other prose/reference Markdown are exempt from the numeric line limit.
- At ~400 lines in a source-code file, evaluate a split before adding another major responsibility.
- Large documentation should remain structured with headings/indexes and may be split when navigation or responsibility improves, but not merely to satisfy a line count.
- Generated local files are not source and must not be committed unless an explicit project rule says otherwise.

## C++ rules
- Standard: C++20.
- Prefer explicit ownership and value semantics.
- Avoid global mutable state unless it mirrors a documented original hardware/game state and is wrapped behind an interface.
- Prefer `std::span`, `std::array`, `std::vector`, fixed-width integer types and RAII.
- Avoid macros for program logic.
- Avoid inheritance-heavy designs unless a concrete game-system need is proven.
- Avoid premature framework abstractions.
- Keep platform APIs outside game logic.
- Preserve endian/address semantics explicitly when reading ROM data.

## Naming
- C++ names describe meaning, not guessed meaning.
- Unknown routines use address-based names such as `sub_0001234A` until evidence supports a semantic rename.
- Every semantic rename of a reverse-engineered routine should be reflected in `REVERSE_ENGINEERING.md`.
- Hardware constants retain documented original addresses where useful.

## Reverse-engineering discipline
For every translated routine record:
- original address/range;
- known callers/callees when available;
- inputs;
- outputs;
- side effects;
- RAM/VRAM locations touched;
- evidence source;
- confidence level;
- translation status;
- test status;
- unresolved questions.

Confidence labels:
- `CONFIRMED` — verified by code/trace/data and reproduced;
- `HIGH` — strong evidence, minor uncertainty;
- `MEDIUM` — plausible but incomplete evidence;
- `LOW` — working hypothesis only.

## Testing
- Build must remain green after every completed task.
- Deterministic translated routines require unit tests when practical.
- Bugs found in translated behavior should receive regression tests.
- Tests must not require copyrighted ROM content in CI unless a future private mechanism is explicitly approved.
- Tests may use synthetic byte sequences designed to exercise algorithms.

## Documentation
Every meaningful action must update at least one of:
- `WORKLOG.md` — what was done;
- `REVERSE_ENGINEERING.md` — what was learned about the original game;
- `DECISIONS.md` — why architecture/direction changed;
- `FILE_MAP.md` — where responsibilities live;
- `ROADMAP.md` — what milestone moved.

Code comments are not a replacement for project documentation.

## Commits and PRs
Commit messages should use a simple prefix where useful:
- `core:`
- `genesis:`
- `game:`
- `tools:`
- `tests:`
- `docs:`
- `build:`
- `re:` for reverse-engineering discoveries/metadata.

A PR description should include:
1. goal;
2. exact scope;
3. evidence used;
4. tests run;
5. documentation updated;
6. known unknowns;
7. next step.

## Scope control
Do not add any of these before their roadmap milestone without an explicit decision:
- SDL/rendering framework integration;
- audio framework integration;
- ECS framework;
- scripting language replacement;
- HD assets;
- widescreen changes;
- online features;
- Story of Thor 2 code;
- generalized Mega Drive emulator CPU core.

## No silent assumptions
If a value or behavior is uncertain, use an explicit marker in documentation such as:
`UNKNOWN`, `HYPOTHESIS`, or `UNVERIFIED`.

Never turn a hypothesis into a semantic API name without recording why.

## Stop conditions
Pause the current coding path and document the blocker when:
- routine boundaries are uncertain;
- two ROM revisions appear to disagree materially;
- a proposed implementation requires guessing behavior;
- a dependency would alter architecture;
- current code cannot be tested enough to distinguish success from failure.

The response to uncertainty is investigation and documentation, not feature drift.
