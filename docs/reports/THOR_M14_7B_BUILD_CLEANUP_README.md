# M14.7B reproducible build cleanup

This package prepares a user-run cleanup of generated CMake/compiler/test
outputs and the explicitly authorized old calls.jsonl build outputs. It does
not remove raw captures, normalized corpora, session databases, receipts,
reports, source, ROM files, canonical generations, or M14.7B evidence.

The original audited plan is
THOR_M14_7B_BUILD_CLEANUP_PLAN_2026-09-26.jsonl. It contained paths, sizes,
Git state, and safety classification, but no file SHA-256 values. The frozen
manifest THOR_M14_7B_BUILD_CLEANUP_FROZEN_2026-09-26.jsonl preserves its
51,653 safe entries and 17,973,234,988-byte baseline, adds a SHA-256 for every
file, and records the exact allowed build roots and protected identities.
The runner pins the frozen manifest hash
5c4c224387c14833dde8e21230c102b4cbf35086db079a355738aaaeb0c85119; if the
manifest changes, it refuses cleanup.

The full C:\Github\Sega-Thor-M14-7B\build tree is a protected OPEN-evidence
deny-root. This includes the next queued OPEN corpus blob, so no M14.7B capture
can enter the build cleanup candidate set. The current canonical generation
and both database hashes are also pinned. The external ROM path and SHA-256
come from the closed real-capture receipt and are rechecked by the runner.

Run the dry-run from PowerShell first:

    powershell -ExecutionPolicy Bypass -File "C:\Github\Sega-Thor-M14-7B\tools\cleanup_m14_7b_safe_build_artifacts.ps1" -DryRun

Inspect
docs\reports\THOR_M14_7B_BUILD_CLEANUP_DRY_RUN_RESULT.jsonl. Execute mode is
accepted only when that result says READY_FOR_USER_EXECUTION, reports exactly
51,653 matched files / 17,973,234,988 bytes, zero skips, and names the pinned
manifest hash.

To regression-test the Execute authorization and deletion path using only
synthetic files under the system temporary directory, run:

    powershell -ExecutionPolicy Bypass -File "C:\Github\Sega-Thor-M14-7B\tools\cleanup_m14_7b_safe_build_artifacts.ps1" -SelfTest

After reviewing the dry-run result, run:

    powershell -ExecutionPolicy Bypass -File "C:\Github\Sega-Thor-M14-7B\tools\cleanup_m14_7b_safe_build_artifacts.ps1" -Execute

Execute repeats the path, root, reparse-point, Git, file type, size, SHA-256,
ROM, current-map, and OPEN-evidence checks immediately before each file
removal. It uses no force option or fallback. Failed entries are skipped and
recorded with their exact reason. It only attempts non-recursive removal of
directories that are empty after file cleanup. The execution JSONL records
each deleted file's path, size, SHA-256, UTC deletion time, and manifest entry
ID. Physical free-space change is measured separately from logical file bytes.

The script does not run ingestion, builds, CTest, or canonical-map tooling.
After the user reports execution, perform the separate filesystem, result-log,
Git, map self-check, ROM, and OPEN raw SHA audit before resuming ingestion.
