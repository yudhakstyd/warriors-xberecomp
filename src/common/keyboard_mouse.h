#pragma once
#include <cstdint>

#include "core/input.h"

namespace kbm {
void WindowUp(void* hwnd);
bool Message(void* hwnd, uint32_t msg, uintptr_t wp, intptr_t lp, intptr_t* result);
void Pad(int port, input::Pad& pad);
}  // namespace kbm
