# BizHawk 2.11.1 controlled harness diagnosis — 2026-09-12

BIZHAWK CONTROLLED HARNESS: **WORKING**

This is an infrastructure test only. No game reverse engineering, new address
search, new savestate, guest RAM/register writes, manual gameplay or long replay
was performed. No production source, ownership map or milestone was changed.

## Verified inputs and environment

- Executable: `C:\Dev\SegaThorTools\BizHawk-2.11.1-win-x64\EmuHawk.exe`;
  file version `2.11.1.0`, runtime version `2.11.1`, Lua `NLua+Lua`.
- ROM: `C:\Github\Sega-Thor\build\reference\Beyond Oasis (USA).bin`.
  SHA256 `EB19BDA4982366A2FD43D65AB8A7F9709D83A8CC902C14A682C088C16359C263`.
- State: `C:\Dev\SegaThorTools\BizHawk-2.11.1-win-x64\Genesis\State\Beyond Oasis (U) [!].Genplus-gx.QuickSave1.State`.
  SHA256 `7FDE47833CE70A1DF34E75D95C84ED87AFC8470D228AF87967C6BD9DD38B3970`.
- Original input hashes were checked before and after completed runner invocations.
- No EmuHawk process existed at initial inspection. Original configuration had
  `SingleInstanceMode=false`, `RunInBackground=true`, `AcceptBackgroundInput=false`.
- Runtime reported `GEN`; stdout confirmed `Initializing GPGX native` and a
  three-button Genesis controller. No native UI automation tool was used.

## Exact reproducible workflow

The local diagnostic scripts and logs are in ignored
`C:\Github\Sega-Thor\build\bizhawk-controlled-harness`.
Run in PowerShell, using a fresh run name (the default contains a timestamp):

```powershell
& 'C:\Github\Sega-Thor\build\bizhawk-controlled-harness\run.ps1'
```

The runner hashes both inputs, creates a separate configuration per run,
disables single-instance routing, automatic state/movie/Lua loads, autosave
of the last state and rewind, and isolates savestate/SaveRAM output paths.
It preserves the installed GPGX sync settings. It launches a hidden process
with the BizHawk installation as working directory, captures stdout/stderr,
records its PID and existing processes, and waits at most 30 seconds.
Timeout reports the exact PID for diagnosis; it does not kill unrelated processes.
Success requires exit code zero and a freshly generated `result=PASS` log.

The actual launch argument shape, including explicit quotes around spaced paths:

```text
EmuHawk.exe --config="<run directory>\config.ini" --lua="C:\Github\Sega-Thor\build\bizhawk-controlled-harness\controlled.lua" "C:\Github\Sega-Thor\build\reference\Beyond Oasis (USA).bin"
```

`run.ps1` also sets `BH_TEST_LOG`, `BH_TEST_STATE`, `BH_TEST_INPUT=right` and
`BH_TEST_HOLD=0` in the child environment. The Lua sequence is:

1. Log Lua startup, version, `emu.getsystemid()`, ROM metadata and frame.
2. Require `GEN`; read `emu.getregister("M68K PC")` to verify initialized services.
3. Call `savestate.load(exact_absolute_path, true)` and require `true`.
4. Require loaded frame **2117**. Install the known execution/write hooks.
5. Apply an explicit all-false controller table and `emu.frameadvance()` three
   times, reproducing the historical settling interval. Require frame **2120**.
6. Set `joypad.set({ Right = true }, 1)` exactly once and advance exactly one frame.
7. Observe Right via `joypad.getwithmovie()["P1 Right"]` inside the A372 callback.
8. Clear controls, pause, record frame **2121**, four SAT bytes and hook counts;
   flush/close the log and call `client.exitCode(0)`.

There is no timing-based sleep to guess core readiness and no post-input waiting
loop. A cold launch enters Lua at frame 1 in this setup; explicit state load
inside Lua resets that startup progress before any measured work.

## Actual test matrix

| Run | Controller Right during A372 | A372 hits | FF13CC before -> after | Result |
| --- | --- | --- | --- | --- |
| `right-3` | true | 6 | `00000000 -> 00880901` | PASS, exit 0 |
| `right-repeat` | true | 6 | same | PASS, exit 0 |
| `neutral-1` | false | 6 | same | control completed, exit 0 |
| `mixed-1` | false | 6 | same | malformed-input control completed, exit 0 |
| `cli-state` | true | 6 | same | PASS with additional CLI state load |
| `coexist-child` | true | 6 | same | PASS while another EmuHawk was alive |

All rows loaded frame 2117 and measured exactly 2120 -> 2121. The write hook
fired once at FF13CC with value `00880901`, register PC `A374`. The independent
execution callback at address A372 read register PC `A372`.

The full logs for `right-3`, `right-repeat` and `coexist-child` are byte-identical:
SHA256 `E3CBBEDF20367DCA33608422DB77EF34146345B9786C07562BD4595E661CBD2B`.
The coexistence holder PID was 38540; its known test process was stopped after
the child completed. No EmuHawk remained at cleanup inspection.

**Interpretation limit:** frame 2121 reports `emu.islagged() == true` and zero
`event.oninputpoll` callbacks. Right was present in the controller passed toward
the core, but this frame does not prove that the game polled it or reacted to it.
Neutral input produces the identical A372/SAT oracle. Therefore the historical
claim that this transition was *caused by Right* is not supported by this test.
It remains a valid startup/state/hook/frameadvance infrastructure oracle.

## Verified APIs, ordering and process behavior

The installed Lua declaration files and the official **2.11.1 tag** were checked:

- CLI ROM, `--config`, `--lua` and `--load-state` all worked live.
  `--lua` implies `--luaconsole`; a separate console-opening step is unnecessary.
  `--load-slot` exists in this version's parser but was not separately run here.
- `savestate.load(path, true)` worked live. `savestate.loadslot(slot, true)` is
  documented locally, but explicit path is preferable for this exact-file test.
- `emu.frameadvance`, `joypad.set`, `emu.getregister`,
  `memory.read_bytes_as_array(address, length, "M68K BUS")`,
  `event.on_bus_exec(callback, address, name, "M68K BUS")`,
  `event.on_bus_write(callback, address, name, "M68K BUS")`, `client.pause`
  and `client.exitCode` all worked live. Register reads also worked inside hooks.
- MainForm loads CLI ROM/core, then CLI state, then loads Lua in its Shown handler.
  The `cli-state` run entered Lua at frame **2118**, despite CLI state being 2117.
  Thus CLI state load alone is not a frame-exact Lua synchronization barrier.
  Reloading the exact state inside Lua reliably restored frame 2117.
- With `SingleInstanceMode=true`, the existing-instance handler receives forwarded
  arguments but only calls `LoadRom(args[0])`; it does not reprocess the CLI Lua,
  config or state options. This is a source-confirmed hazard, not the cause of
  the initial run here. Live coexistence was tested only with single-instance
  mode disabled and separate configuration files; it succeeded.

Primary version-specific sources:
[ArgParser.cs](https://github.com/TASEmulators/BizHawk/blob/2.11.1/src/BizHawk.Client.Common/ArgParser.cs),
[MainForm.cs](https://github.com/TASEmulators/BizHawk/blob/2.11.1/src/BizHawk.Client.EmuHawk/MainForm.cs),
[JoypadApi.cs](https://github.com/TASEmulators/BizHawk/blob/2.11.1/src/BizHawk.Client.Common/Api/Classes/JoypadApi.cs).
Relevant source areas: MainForm lines 700-817 and 4654-4793; JoypadApi lines 52-84.

## Root cause and Codex/Luna correction

The reported CUA error is a limitation of the chosen native UI tool, not a
BizHawk runtime blocker. Use the process launcher and the native CLI/Lua APIs.
No claim is made about Luna's exact prior command line, which was not provided.

A concrete existing-probe defect was independently confirmed:
`joypad.set({ ["P1 Right"] = true }, 1)` does **not** assert Right in this version.
With a port supplied, keys must be unprefixed: `joypad.set({ Right = true }, 1)`.
Alternatively omit the port: `joypad.set({ ["P1 Right"] = true })`.
The latter spelling is source-supported; the port-specific spelling was tested.
The old source was left unchanged because this task is independent diagnosis.

Two early diagnostic assertions were corrected with evidence: `gameinfo.getromhash()`
is not a SHA256/SHA1 identity API here (it returned
`5A79DE198ED6B1ECE2518574FE78D962`); input identity is checked by the host SHA256.
Also, zero input-poll callbacks on the measured lag frame do not mean Lua failed
to set the controller. Actual controller state is now checked inside execution.
The early FAIL logs (`right-1`, `right-2`) are retained for audit.

## Next minimal test and validation boundary

For infrastructure regression, rerun `run.ps1` and require the same frame,
controller, callback and SAT values. No further replay is needed for this task.
Any future gameplay-input causality test must separately observe an input poll;
do not treat this lag-frame SAT transition as proof of movement.

ROM and existing state remain hash-identical. No `.State` was created.
BizHawk normally flushes SaveRAM on exit even with periodic autosave disabled;
those outputs were isolated in ignored run directories. Raw logs/configs and
upstream source downloads are also ignored and are not repository deliverables.
Runtime tests replace native C++ rebuilds here because no C++/CMake code changed.
No native build, deployment or CI execution was performed for this
docs/runtime-only change.

Final verification: `git diff --check`, PowerShell parser validation and a
complete tracked-source 500-line scan passed. Local harness files have 106 Lua
and 81 PowerShell lines. The stock CMake size script was started but its broad
recursive workspace glob was stopped without a result; the completed tracked
scan plus explicit harness counts are the reported size verification.
