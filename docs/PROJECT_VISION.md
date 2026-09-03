# Project Vision

## Idea
Build a clean, portable C++20 reimplementation of **Beyond Oasis / The Story of Thor** for modern systems while preserving original gameplay behavior.

The repository contains only our source code, tests, reverse-engineering notes, and tooling. Original ROM data remains external and is supplied locally by the user.

## What this project is
- A reverse-engineering project.
- A native C++ reimplementation of game logic.
- A compatibility/runtime layer for the hardware behavior the game actually needs.
- A tooling project for inspecting and extracting data from a legally obtained ROM.
- A documentation-first effort where evidence and uncertainty are tracked.

## What this project is not
- Not a ROM distribution.
- Not a copyrighted asset archive.
- Not a general Mega Drive emulator.
- Not a remake that changes gameplay before parity.
- Not a Story of Thor 2 project yet.

## Core principles
### 1. Fidelity first
Before adding enhancements, reproduce original behavior as closely as practical.

### 2. Evidence over guesses
Unknown behavior must be labeled unknown. Reverse-engineered claims should reference addresses, traces, data layouts, or reproducible observations.

### 3. Small verified steps
Translate one routine/system at a time, test it, document it, then continue.

### 4. Modern structure, original semantics
The C++ architecture may be cleaner than the original assembly, but observable game behavior should remain compatible.

### 5. ROM-owned data stays outside Git
Tools may read and transform local ROM data at runtime/build time. Extracted commercial assets are not committed.

## End-state
A user should be able to provide an accepted Beyond Oasis ROM and run a native executable on modern operating systems without requiring a full Mega Drive emulator.

Later phases may add optional quality-of-life features, modern rendering, additional resolutions, controller support, and platform packaging. These remain secondary to completing a faithful base implementation.
