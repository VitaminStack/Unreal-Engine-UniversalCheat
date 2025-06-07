#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include "MinHook/MinHook.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_dx12.h"
#include "ImGui/imgui_impl_win32.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "MinHook/libMinHook.x64.lib")

class DX12Hook {
public:
    DX12Hook();
    ~DX12Hook();

    bool Init();
    void Shutdown();

    // Muss im Hooked Present aufgerufen werden
    void OnRender(IDXGISwapChain3* pSwap, ID3D12GraphicsCommandList* pCmdList);

private:
    HWND hWnd = nullptr;
    bool imguiInitialized = false;
    ID3D12DescriptorHeap* imguiSrvDescHeap = nullptr;
};