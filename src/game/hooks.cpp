// Hooks for default.xbe. A strong definition replaces the recompiled function at that address.
#include <chrono>
#include <cstdint>

#include "xdk/hle.h"

// DSOUND's CMcpxCore time query (thiscall, one stack argument: a 64-bit output pointer). The original reads the
// APU's timer register at 0xFE80200C, which the host does not map, then turns the 32-bit tick count into elapsed
// 48 kHz samples. Replaced by the host clock: samples elapsed since the first call.
// Found by the guest trap at 0x003D4D0D (round 1 of the audio fixes).
X86_FUNC(sub_003D4CF6) {
    (void)base;
    using namespace std::chrono;
    static const auto start = steady_clock::now();
    const uint64_t us = (uint64_t)duration_cast<microseconds>(steady_clock::now() - start).count();
    const uint64_t samples = us * 48000ull / 1000000ull;
    const uint32_t out = ::kernel::R32(ctx.esp + 4);
    ::kernel::W32(out, (uint32_t)samples);
    ::kernel::W32(out + 4, (uint32_t)(samples >> 32));
    ::kernel::Return(ctx, 0, 1);
}


// IDirectSound::CreateSoundStream(pDS, pdssd, ppStream, pUnkOuter): the title's XACT engine calls this method form,
// which the SDK does not replace (only the DirectSoundCreateStream function). Forwarded to the host's
// DirectSoundCreateStream(pdssd, ppStream). Round 5 of the audio fixes.
X86_FUNC(sub_003AD1C1) {
    (void)base;
    const uint32_t args[2] = {::kernel::R32(ctx.esp + 8), ::kernel::R32(ctx.esp + 12)};
    const uint32_t r = ::kernel::CallGuest(ctx, SYM(DirectSoundCreateStream), args, 2);
    ::kernel::Return(ctx, r, 4);
}

// IDirectSoundStream::SetOutputBuffer(pStream, pOutputBuffer): the SDK handles it for buffers only. A stream keeps the
// host mixer's own mix bins, as the buffer version does (S_OK, two stack arguments). Round 6 of the audio fixes.
X86_FUNC(sub_003ABF36) {
    (void)base;  // round 13: forwarded to the buffer version, which now follows the submix routing for any voice
    const uint32_t args[2] = {::kernel::R32(ctx.esp + 4), ::kernel::R32(ctx.esp + 8)};
    const uint32_t r = ::kernel::CallGuest(ctx, SYM(IDirectSoundBuffer_SetOutputBuffer), args, 2);
    ::kernel::Return(ctx, r, 2);
}

// A stream export stub (0x003AA0E2 -> 0x003A9B98) that XACT calls as (pStream, flag) and that ends in a voice
// status bit the host mixer does not have. Treated as a no-op returning S_OK. Round 7 of the audio fixes.
X86_FUNC(sub_003AA0E2) {
    (void)base;
    ::kernel::Return(ctx, 0, 2);
}

// IDirectSoundBuffer::PlayEx(pBuffer, rtTimeStamp (two dwords), dwFlags): forwarded to the host's Play(pBuffer, 0, 0,
// dwFlags); the time stamp is ignored. Round 8 of the audio fixes.
X86_FUNC(sub_003A9EC4) {
    (void)base;
    const uint32_t args[4] = {::kernel::R32(ctx.esp + 4), 0, 0, ::kernel::R32(ctx.esp + 16)};
    const uint32_t r = ::kernel::CallGuest(ctx, SYM(IDirectSoundBuffer_Play), args, 4);
    ::kernel::Return(ctx, r, 4);
}

// Two DSOUND exports (buffer 0x003AA02C, stream 0x003AA0D8) that XACT calls as (pVoice, pOut) to read the voice's
// current parameter list: a count followed by {type, value} pairs. The host keeps no such list, so the count is
// written as 0; left alone the count was garbage and the caller walked off the end of the stack. Round 9.
X86_FUNC(sub_003AA02C) {
    (void)base;
    ::kernel::W32(::kernel::R32(ctx.esp + 8), 0);
    ::kernel::Return(ctx, 0, 2);
}
X86_FUNC(sub_003AA0D8) {
    (void)base;
    ::kernel::W32(::kernel::R32(ctx.esp + 8), 0);
    ::kernel::Return(ctx, 0, 2);
}

// Buffer export 0x003AA064 (-> 0x003A986A -> 0x003A85EC), first mapped as IDirectSoundBuffer_Pause. XACT calls it right
// after IDirectSoundBuffer_SetOutputBuffer (0x003D3408) with 1 when the voice, or the submix buffer it is routed
// into, has flag 0x02: it sets a routing bit of the console voice, not a pause. As Pause it held every voice routed
// through a submix (punches, footsteps) silent. The host mixes voices straight to their bins, so it is a no-op, like
// its stream twin 0x003AA0E2. Round 12 of the audio fixes.
X86_FUNC(sub_003AA064) {
    (void)base;
    ::kernel::Return(ctx, 0, 2);
}
