#include "app/app.h"
#include "kernel/kernel.h"

// DSOUND initialises these in code the host replaces (DirectSoundCreate), so the guest would call null pointers:
// the 3D positioning routines (the ten routines DirectSoundUseLightHRTF installs, written by the function at 0x003AFC2B), and
// two pointers to constant tables (written at 0x003AF108 / 0x003AF113). Rounds 4 and 5 of the audio fixes.
static void WarriorsLoaded() {
    static const uint32_t kRoutines[10] = {0x3AF23B, 0x3AF284, 0x3AF3E7, 0x3AF47D, 0x3AF4C8,
                                           0x3AF5D9, 0x3AF669, 0x3AF738, 0x3AF7DA, 0x3AF853};
    for (int i = 0; i < 10; i++) kernel::W32(0x3CD868 + 4 * i, kRoutines[i]);
    kernel::W32(0x3CD890, 3);  // what DirectSoundUseLightHRTF (0x003AFC2B) stores last
    kernel::W32(0x3CD864, 0x3CD184);
    kernel::W32(0x3CD860, 0x3CD188);
    // IDirectSound_GetCaps (0x003A9C16 -> 0x003A7DA6) reports the console's free voice counts from these globals,
    // which DSOUND keeps as it makes and frees hardware voices. The host makes the voices, so they stayed 0 and XACT
    // saw no free 3D voice: positional sounds (footsteps, punches) were never started. Set to the console's totals.
    kernel::W32(0x3CD194, 192);   // dwFree2DBuffers
    kernel::W32(0x3CD198, 64);    // dwFree3DBuffers
    kernel::W32(0x3CD19C, 2047);  // dwFreeBufferSGEs
}

// what the runtime asks of the game; strings must be set, hooks may stay null
const app::GameInfo app::kGame = {
    .title = L"The Warriors",               // window and splash
    .short_title = L"Warriors",               // message boxes
    .name = L"Warriors",                      // window classes, crash dumps, handover event
    .log = "warriors",                        // log file prefix
    .settings = "settings.toml",          // settings file beside the exe
    .splash = L"splash",                  // optional splash.png or splash.bmp beside the exe
    .missing = L"Put the game in the assets folder: its disc image (.iso), or default.xbe with its folders beside it.",
    .images = {{"default.xbe", "Warriors"}},  // each recompiled xbe and its exe, for title launches
    .options = {},                        // game settings kept in the settings file
    .load = nullptr,                      // reads more from the settings file's toml::table
    .save = nullptr,                      // writes more into it
    .init = nullptr,                      // after the settings load, before the window and boot
    .loaded = WarriorsLoaded,                    // image in memory, before its entry point runs
    .window = nullptr,                    // the game window is up
    .message = nullptr,                   // window messages first; true takes one
    .ui = nullptr,                        // ImGui drawing each frame
    .pad = nullptr,                       // changes a polled pad
    .rumble = nullptr,                    // changes a rumble before it is set
};
