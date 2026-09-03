# Porting notes

The project is intentionally split between game logic and Mega Drive compatibility code.

## Rule

Do not commit copyrighted ROM images or extracted original game assets.

## First reverse-engineering target

`0x00003820` — graphics decompression routine identified by existing public ROM-hacking work.

The plan is to recover its input/output contract, translate it to C++, and validate output against the original game behavior.
