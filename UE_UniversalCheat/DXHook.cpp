#include "DXHook.h"
#include <cstdio>

// Original Present Zeiger
typedef HRESULT(__stdcall* PresentDX12)(IDXGISwapChain3*, UINT, UINT);
PresentDX12 oPresentDX12 = nullptr;

// Globale Instanz
DX12Hook* g_pDX12Hook = nullptr;

// Helper: Erstellt Descriptor Heap für ImGui
ID3D12DescriptorHeap* CreateImGuiDescriptorHeap(ID3D12Device* device) {
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 1;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ID3D12DescriptorHeap* heap = nullptr;
    if (FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)))) return nullptr;
    return heap;
}

// Unser Present-Hook
HRESULT __stdcall hkPresentDX12(IDXGISwapChain3* pSwap, UINT sync, UINT flags) {
    // CommandList aus aktuellem Frame holen (hier nur Dummy, normalerweise müsstest du sie aus dem CommandQueue-Hook merken!)
    ID3D12GraphicsCommandList* pCmdList = nullptr;

    // Minimal-Render
    if (g_pDX12Hook)
        g_pDX12Hook->OnRender(pSwap, pCmdList);

    return oPresentDX12(pSwap, sync, flags);
}

// --- DX12Hook Methoden ---

DX12Hook::DX12Hook() {
    g_pDX12Hook = this;
}
DX12Hook::~DX12Hook() {
    Shutdown();
}

bool DX12Hook::Init() {
    // Dummy: Warte auf echtes SwapChain3-Objekt. In der echten Anwendung hookst du über Patternscan/Detour nach Init!
    // Hier nehmen wir an, du hast den Zeiger schon (z.B. via DLL Injection im Spiel mit DX12).
    // Wir machen einen Patternscan oder Signature-Scan und hooken das Present.
    // Für dieses Demo: Erwarte, dass SwapChain3 schon bereit ist (Handle musst du im echten Spiel holen!).

    // Beispiel: Du hast die Adresse der Present-Funktion via VMT-Scan.
    // Annahme: DummySwapChain ist ein gültiges IDXGISwapChain3*
    IDXGISwapChain3* dummySwap = nullptr; // Du musst einen echten Zeiger besorgen!
    if (!dummySwap) {
        printf("DX12Hook: No swapchain for hook!\n");
        return false;
    }

    void** vtable = *(void***)(dummySwap);
    void* presentAddr = vtable[8]; // Present ist Index 8
    if (MH_CreateHook(presentAddr, &hkPresentDX12, reinterpret_cast<LPVOID*>(&oPresentDX12)) != MH_OK)
        return false;
    if (MH_EnableHook(presentAddr) != MH_OK)
        return false;

    printf("DX12Hook: Hooked Present!\n");
    return true;
}

void DX12Hook::Shutdown() {
    // Unhook, etc.
    MH_DisableHook(MH_ALL_HOOKS);
    g_pDX12Hook = nullptr;
}

void DX12Hook::OnRender(IDXGISwapChain3* pSwap, ID3D12GraphicsCommandList* pCmdList) {
    if (!pSwap)
        return;

    if (!imguiInitialized) {
        DXGI_SWAP_CHAIN_DESC sd;
        pSwap->GetDesc(&sd);
        hWnd = sd.OutputWindow;

        ID3D12Device* device = nullptr;
        if (FAILED(pSwap->GetDevice(__uuidof(ID3D12Device), (void**)&device)) || !device) {
            printf("DX12Hook: Device missing!\n");
            return;
        }
        imguiSrvDescHeap = CreateImGuiDescriptorHeap(device);

        ImGui::CreateContext();
        ImGui_ImplWin32_Init(hWnd);
        ImGui_ImplDX12_Init(
            device, 2, DXGI_FORMAT_R8G8B8A8_UNORM,
            imguiSrvDescHeap,
            imguiSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
            imguiSrvDescHeap->GetGPUDescriptorHandleForHeapStart()
        );
        imguiInitialized = true;
    }

    if (!imguiSrvDescHeap || !pCmdList)
        return;

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("DX12 Overlay");
    ImGui::Text("Hello from DX12 ImGui!");
    ImGui::Text("Time: %.2f", ImGui::GetTime());
    ImGui::End();

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pCmdList);
}
