// DirectSound exports that XACT calls on the host's buffers and streams but that the SDK has no replacement for.
// Their original bodies write into the console's voice structures, which the host's objects do not have (null
// pointers inside), so each is a no-op returning S_OK, popping its stdcall arguments (found from the thunks'
// "ret N", which `this` plus its parameters make up). Effects: no per-voice 3D cone/doppler/LFO/envelope settings.
// Addresses are the export thunks listed in docs of this trial; 0x003A9EC4 (PlayEx) is forwarded in hooks.cpp.
#include <cstdint>

#include "xdk/hle.h"

#define DSOUND_NOOP(addr, ret_bytes)     X86_FUNC(sub_##addr) {                   (void)base;                          ::kernel::Return(ctx, 0, (ret_bytes) / 4);     }

// buffer exports
DSOUND_NOOP(003A9DF8, 0x8)
DSOUND_NOOP(003A9E14, 0x8)
DSOUND_NOOP(003A9E30, 0x8)
DSOUND_NOOP(003A9E4C, 0x8)
DSOUND_NOOP(003A9F24, 0x8)
DSOUND_NOOP(003A9F40, 0x10)
DSOUND_NOOP(003ABCF6, 0xc)
DSOUND_NOOP(003ABD16, 0x10)
DSOUND_NOOP(003ABD3A, 0x14)
DSOUND_NOOP(003ABD6F, 0xc)
DSOUND_NOOP(003ABDD7, 0xc)
DSOUND_NOOP(003ABE61, 0xc)
DSOUND_NOOP(003ABE85, 0xc)
DSOUND_NOOP(003ABEA9, 0xc)
DSOUND_NOOP(003ABECD, 0x10)
// stream exports
DSOUND_NOOP(003AA085, 0x8)
DSOUND_NOOP(003AA08A, 0x8)
DSOUND_NOOP(003AA08F, 0x8)
DSOUND_NOOP(003AA0DD, 0x8)
