# Standalone Handoff: M15.301 — Sound Driver 11-Channel FM/PSG/SFX RAM Map (`0x0604E6`..`0x06064B`)

## Summary
The Beyond Oasis 68K sound driver (`0x060000..`) manages **11 audio channels** in lower WRAM (`$FF0022..$FF07BB`):
- **6 FM Channels** (`0xCA` = `202`-byte stride): `$FF0022`, `$FF00EC`, `$FF01B6`, `$FF0280`, `$FF034A`, `$FF0414`.
- **3 PSG Channels** (`0x8A` = `138`-byte stride): `$FF049E`, `$FF0528`, `$FF05B2`.
- **2 SFX Channels** (`0xCA` = `202`-byte stride): `$FF0628` (SFX FM), `$FF06F2` (SFX PSG).
- **System Cmd `0x00` (`0x0604E6`):** Stop/Mute all channels (`0x0611EA`, clears `$FF0010..$FF0014`).
- **System Cmd `0x07` (`0x060516`):** Sets fade step `2`, timer `0x7E`, and sets fade bit 4 on `+0x75` across all 11 channels.

## C++20 Standalone Target
- Extend `src/game/audio/` with 11-channel `SoundChannelBank` (`6 FM + 3 PSG + 2 SFX`) and `stop_all()` / `fade_all()` commands.
- Evidence artifact: `data/thor-evidence/rom_sound_channels_m15_301.json`.
