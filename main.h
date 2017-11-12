#pragma once

#include "common.h"

namespace MRT {
    enum WindowEvent {
        MRT_CLOSE,
        // TODO!
    };

    enum KeyState {
        MRT_UP = -1,
        MRT_NONE = 0,
        MRT_DOWN = 1,
    };

    void MouseCallback(int32 x, int32 y, KeyState lButton, KeyState rButton);
    void KeyboardCallback(int keycode, KeyState state, KeyState prev);
    void WindowCallback(WindowEvent e);
}