#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <tchar.h>
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_impl_dx11.h"
#include "../ImGui/imgui_impl_win32.h"
#include "../Helper/Helper.h"
#include "../Helper/Cheese.h"
#include "../Hooks/menu/menu.hpp"

#pragma comment(lib, "dwmapi.lib")
#include "Dwmapi.h"



// DirectX11 Variablen
extern ID3D11Device* g_pd3dDevice;
extern ID3D11DeviceContext* g_pd3dDeviceContext;
extern IDXGISwapChain* g_pSwapChain;
extern ID3D11RenderTargetView* g_mainRenderTargetView;
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


extern bool running;
extern HWND hWndOverlay;
extern int Screen_w;
extern int Screen_h;



// Funktionen
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void RenderOverlay();