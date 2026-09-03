# Sega Thor / Beyond Oasis C++

Reverse-engineering and native C++ reimplementation project for **Beyond Oasis / The Story of Thor** (Sega Mega Drive / Genesis).

## Goal

Reimplement the original game logic in portable C++ while using data extracted from a user-supplied original ROM. This repository must not contain copyrighted ROM images or original game assets.

## Current status

Initial bootstrap:

- C++20 project
- ROM loader
- Mega Drive memory map scaffold
- VDP/VRAM scaffold
- known reverse-engineered symbol addresses
- first translated tile copy helpers

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## ROM policy

Do not commit ROM files. The runtime/tooling will operate on a locally supplied, legally obtained ROM dump.

## Reverse-engineering notes

Known starting points from existing public ROM-hacking work include:

- VDP data port: `0xC00000`
- VDP control port: `0xC00004`
- 68000 RAM base: `0xFF0000`
- graphics decompression routine: `0x00003820`

The next milestone is to recover and translate the graphics decompression routine into tested C++.
