#include "Overlay.h"

// DirectX11 Variablen
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// Overlay Fenster-Variablen

static bool PawnFilterEnabled = true;
static bool vsyncEnabled = false;

int Screen_w = 2560;
int Screen_h = 1440;
bool running = true;
float fps = 100.f;
FPSLimiter fpsLimiter(60.0f);
bool openMenu = false;
bool Clickability = false;
HWND hWndOverlay;
static int selectedLevelIndex = 0;  // Aktuell ausgewählter Level
static std::vector<std::string> levelNames;  // Liste der Levelnamen



void ChangeClickability(bool canClick, HWND hwnd) {
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

    if (canClick) {
        SetWindowLong(hwnd, GWL_EXSTYLE, exStyle & ~WS_EX_TRANSPARENT);  // Klickbar
        SetForegroundWindow(hwnd);  // Bringt das Fenster in den Vordergrund (optional)
    }
    else {
        SetWindowLong(hwnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);   // Klicks durchlassen
    }
}
void SetOverlayToTarget(HWND WindowHandle, HWND OverlayHandle)
{
    RECT rect{};
    if (!GetWindowRect(WindowHandle, &rect))
        return;

    Screen_w = rect.right - rect.left;
    Screen_h = rect.bottom - rect.top;

    MoveWindow(OverlayHandle, rect.left, rect.top, Screen_w, Screen_h, TRUE);
}

// DirectX11 Geräteerstellung
bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

    HRESULT res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT, featureLevels, 1, D3D11_SDK_VERSION, &sd,
        &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);

    if (FAILED(res)) return false;

    CreateRenderTarget();
    return true;
}
// Render Target erstellen
void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}
// Ressourcen aufräumen
void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}
void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}


// Fenster-Callback
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

// Hauptfunktion für das Overlay
void RenderOverlay()
{
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), ACS_TRANSPARENT, WindowProc, 0L, 0L, GetModuleHandle(NULL), NULL, LoadCursor(NULL, IDC_ARROW), NULL, NULL, _T("Overlay"), NULL };
    RegisterClassEx(&wc);
    HWND targetWindow = GetForegroundWindow();
    HWND hWndOverlay = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,  // Layered + Transparent
        wc.lpszClassName,
        L"Window",
        WS_POPUP,
        0, 0, Screen_w, Screen_h,
        nullptr, nullptr, nullptr, nullptr
    );
    SetLayeredWindowAttributes(hWndOverlay, RGB(0, 0, 0), 0, LWA_COLORKEY);
    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(hWndOverlay, &margins);

    if (!CreateDeviceD3D(hWndOverlay))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return;
    }

    ::ShowWindow(hWndOverlay, SW_SHOWDEFAULT);
    ::UpdateWindow(hWndOverlay);

    if (targetWindow)
        SetOverlayToTarget(targetWindow, hWndOverlay);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_None;
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hWndOverlay);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    HWND consoleWindow = GetConsoleWindow();
    MSG msg;
    while (running)
    {
        bool consoleActive = GetForegroundWindow() == consoleWindow;
        if (consoleActive)
        {
            ShowWindow(hWndOverlay, SW_HIDE);
        }
        else
        {
            ShowWindow(hWndOverlay, SW_SHOW);
            if (targetWindow)
                SetOverlayToTarget(targetWindow, hWndOverlay);
            SetWindowPos(hWndOverlay, HWND_TOPMOST, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
        // ✅ Windows-Nachrichten verarbeiten
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                running = false;
        }
        if (!running) break;

        // ✅ Menü mit VK_INSERT öffnen/schließen
        if (GetAsyncKeyState(VK_INSERT) & 1) {
            Clickability = !Clickability;
            openMenu = !openMenu;
        }

        ChangeClickability(Clickability, hWndOverlay);
        ImGuiIO& IO = ImGui::GetIO();
        fpsLimiter.cap(IO);

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();



        Menu::Render();

        ImGui::SliderFloat("FPS", &fps, 1.f, 170.f);
        fpsLimiter.setTargetFPS(fps);

        // ✅ Rendering
        float TransparentColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, TransparentColor);
        ImGui::Render();

        const float clear_color_with_alpha[4] = {
            clear_color.x * clear_color.w,
            clear_color.y * clear_color.w,
            clear_color.z * clear_color.w,
            clear_color.w
        };

        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(vsyncEnabled ? 1 : 0, 0);
    }

    // **Schließe Overlay korrekt**
    PostMessage(hWndOverlay, WM_CLOSE, 0, 0);
    Sleep(100);
    DestroyWindow(hWndOverlay);
    UnregisterClass(wc.lpszClassName, wc.hInstance);

    // **DirectX-Ressourcen freigeben**
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
}