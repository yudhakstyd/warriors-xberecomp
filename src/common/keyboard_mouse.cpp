// Keyboard and mouse as the console controller on port 1, through GameInfo::pad. Read while the game window has the
// focus; a real controller on the port keeps working and the keys add to it.
#include "common/keyboard_mouse.h"

#include <algorithm>
#include <atomic>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace {

std::atomic<HWND> g_window{nullptr};
POINT g_last{};
bool g_have_last = false;
std::atomic<bool> g_split{false};
bool g_f9_was_down = false;
thread_local bool g_nested = false;

bool Down(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }

int16_t Axis(bool negative, bool positive) { return int16_t((positive ? 32767 : 0) + (negative ? -32767 : 0)); }

// pixels moved since the last read into stick deflection: 12 px is a full push
int16_t Look(long delta) { return int16_t(std::clamp(delta * 32767 / 12, -32767L, 32767L)); }

}  // namespace

namespace kbm {

void WindowUp(void* hwnd) { g_window = static_cast<HWND>(hwnd); }

// the cursor stays hidden over the window while it has the focus
bool Message(void* hwnd, uint32_t msg, uintptr_t, intptr_t lp, intptr_t* result) {
    if (msg == WM_SETCURSOR && LOWORD(lp) == HTCLIENT && GetForegroundWindow() == static_cast<HWND>(hwnd)) {
        SetCursor(nullptr);
        *result = TRUE;
        return true;
    }
    return false;
}

void Pad(int port, input::Pad& pad) {
    if (g_nested) return;  // the read below for the pad one port down
    HWND hwnd = g_window.load();
    const bool focus = hwnd && GetForegroundWindow() == hwnd;

    // F9 flips the two-player test: the keyboard and mouse stay player 1 and the real controllers move up a port
    if (port == 0 && focus) {
        const bool f9 = Down(VK_F9);
        if (f9 && !g_f9_was_down) g_split = !g_split;
        g_f9_was_down = f9;
    }
    if (g_split) {
        if (port > 0 && port < 4) {
            g_nested = true;
            pad = input::Poll(port - 1);
            g_nested = false;
            return;
        }
        if (port == 0) pad = input::Pad{};
    }

    if (port != 0 || !focus) {
        g_have_last = false;
        return;
    }

    // mouse: movement is the right stick (camera), the cursor is put back in the middle of the window each read
    RECT rc;
    POINT pos;
    long dx = 0, dy = 0;
    if (GetClientRect(hwnd, &rc) && GetCursorPos(&pos)) {
        POINT mid = {(rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2};
        ClientToScreen(hwnd, &mid);
        if (g_have_last) {
            dx = pos.x - mid.x;
            dy = pos.y - mid.y;
        }
        SetCursorPos(mid.x, mid.y);
        g_have_last = true;
    }

    const bool left = Down('A'), right = Down('D'), fwd = Down('W'), back = Down('S');
    const bool dpad_up = Down(VK_UP), dpad_down = Down(VK_DOWN), dpad_left = Down(VK_LEFT), dpad_right = Down(VK_RIGHT);

    uint16_t buttons = 0;
    buttons |= dpad_up ? 0x01 : 0;
    buttons |= dpad_down ? 0x02 : 0;
    buttons |= dpad_left ? 0x04 : 0;
    buttons |= dpad_right ? 0x08 : 0;
    buttons |= Down(VK_ESCAPE) ? 0x10 : 0;                // start
    buttons |= Down(VK_BACK) || Down(VK_TAB) ? 0x20 : 0;  // back
    buttons |= Down('Q') ? 0x40 : 0;                      // left stick click
    buttons |= Down(VK_MBUTTON) ? 0x80 : 0;               // right stick click

    uint8_t analog[8] = {};
    analog[0] = Down(VK_SPACE) || Down(VK_RETURN) ? 0xFF : 0;   // A
    analog[1] = Down('E') ? 0xFF : 0;                            // B
    analog[2] = Down(VK_LBUTTON) || Down('J') ? 0xFF : 0;        // X
    analog[3] = Down(VK_RBUTTON) || Down('F') ? 0xFF : 0;        // Y
    analog[4] = Down('R') ? 0xFF : 0;                            // black
    analog[5] = Down('C') ? 0xFF : 0;                            // white
    analog[6] = Down(VK_SHIFT) ? 0xFF : 0;                       // left trigger
    analog[7] = Down(VK_CONTROL) || Down('X') ? 0xFF : 0;        // right trigger

    int16_t lx = Axis(left, right), ly = Axis(back, fwd);
    if (lx && ly) {  // diagonals are no faster than straight
        lx = int16_t(lx * 7071 / 10000);
        ly = int16_t(ly * 7071 / 10000);
    }
    const int16_t rx = Look(dx), ry = Look(-dy);

    const bool any = buttons || lx || ly || rx || ry || std::any_of(analog, analog + 8, [](uint8_t a) { return a; });

    if (!pad.connected) pad.connected = true;  // keyboard and mouse are always plugged in
    if (!any) return;
    pad.buttons |= buttons;
    for (int i = 0; i < 8; i++) pad.analog[i] = std::max(pad.analog[i], analog[i]);
    if (lx) pad.lx = lx;
    if (ly) pad.ly = ly;
    if (rx) pad.rx = rx;
    if (ry) pad.ry = ry;
}

}  // namespace kbm
