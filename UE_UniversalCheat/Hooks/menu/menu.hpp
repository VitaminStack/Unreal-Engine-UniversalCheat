#pragma once

#include <Windows.h>

namespace Menu {
    void InitializeContext(HWND hwnd);
    void Render();
    void RenderGameMenu();
    void RenderDebugMenu();

    inline bool bShowMenu = true;
} // namespace Menu
