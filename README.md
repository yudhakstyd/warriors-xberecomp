# The Warriors - XBE Recomp

An unofficial native PC port of the original Xbox version of The Warriors (2005), made by static recompilation with
[Xbox XBE Recomp](https://github.com/TGP482/XboxXBE-Recomp). The game's x86 code is translated to C++ ahead of time and
runs on Windows with the SDK's Direct3D 12 renderer, XAudio2 audio and SDL3 input.

> [!NOTE]
> This repository contains no game files, keys or recompiled code. You need your own copy of the game: its
> `default.xbe` and a disc image. Builds made from it are for your own use.

## Status

- Boots, plays the intro movies, menus and the first level (tutorial) with a gamepad.
- Audio works: music, dialogue, cutscene audio stopping on skip, punches and footsteps.
- Not done: keyboard and mouse controls, some menu objects render partly black, only the start of the game has
  been played.

Tested with the USA disc (`default.xbe` SHA-1 `fe246a74501cd10d2ce89ca5dbda5080d2b2edcc`, title id `5454009B`,
XDK 5849).

## Build

Requirements: Windows 10/11 64-bit, Visual Studio 2022 (or Build Tools) with "Desktop development with C++",
"C++ Clang tools for Windows" and "C++ CMake tools for Windows", and git.

1. Clone with the SDK submodule:
   ```
   git clone --recursive https://github.com/yudhakstyd/warriors-xberecomp.git
   ```
2. Put `default.xbe` from your disc in `xbe\`, and your disc image (`.iso`, game partition or full dump) in `assets\`.
   `default.xbe` must **not** be in `assets\` when you play: the runtime would then run without mounting the disc.
3. Run `build.bat`. It applies `patches\` to the SDK, copies `default.xbe` into `assets\` for the build only, and
   recompiles the game (the first build takes 30 to 90 minutes).
4. Run `out\build\release\Warriors.exe`.

## What this port changes

`config\game.toml` is the recompiler input `xbe_symbols` wrote, plus fixes by hand (`[[function]]`, the
`XPP_GamepadType` address and DirectSound export addresses the signatures did not find). Hooks are in `src\game`,
game setup in `src\common\game_info.cpp`.

Most of the work is audio. The title runs Microsoft's XACT engine (its own code, recompiled) on top of the SDK's host
DirectSound, and XACT relies on details of the console's sound hardware:

| Problem | Fix |
|---|---|
| DSOUND reads the APU timer register | `sub_003D4CF6` returns elapsed samples from the host clock |
| DirectSound exports the signatures missed (`SetOutputBuffer`, `SetEG`, `SetFormat`, `SetPlayRegion`, `StopEx`, `SynchPlayback`, `CommitEffectData`, stream `AddRef`/`Release`) | addresses in `config\game.toml` (stream ones read from the stream vtable at `0x447B88`) |
| Exports with no host version (`IDirectSound::CreateSoundStream`, `PlayEx`, stream `SetOutputBuffer`, parameter-list readers, routing bits) | forwarded or replaced in `src\game\hooks.cpp` |
| Other buffer and stream exports XACT calls whose bodies touch console voice memory | no-ops in `src\game\dsound_noops.cpp` |
| DSOUND pointer tables and free-voice counts its replaced init would fill | written when the image loads (`game_info.cpp`) |

Runtime changes needed by this title, in `patches\0001-dsound-submix-routing-and-xact-streams.patch` (against SDK
commit `4a4d0f0`):

- voices routed into a submix buffer (`IDirectSoundBuffer_SetOutputBuffer`) take that buffer's mix bins; without it
  punches and footsteps went to the reverb send and the subwoofer only;
- stream `GetStatus` reports the console's bits (packets queued, playing, paused, starved) instead of "accepts input",
  so XACT sees a stopped stream go idle and ends the cue;
- `DSSTREAMPAUSE` 3 (pause without activating) pauses, and an asynchronous `FlushEx` also flushes packets still in
  flight, so skipped cutscene dialogue stops.

## Credits

[Xbox XBE Recomp](https://github.com/TGP482/XboxXBE-Recomp) by TGP482 (MIT), which this port is built with.
