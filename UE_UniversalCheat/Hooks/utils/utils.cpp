#include <Windows.h>
#include <thread>
#include <dxgi.h>

#include "utils.hpp"
#include <vector>
#include <string>
#include <psapi.h>
#include "../../Helper/Logger.h"
#include "../backend/dx9/hook_directx9.hpp"
#include "../backend/dx10/hook_directx10.hpp"
#include "../backend/dx11/hook_directx11.hpp"
#include "../backend/dx12/hook_directx12.hpp"
#include "../backend/opengl/hook_opengl.hpp"
#include "../backend/vulkan/hook_vulkan.hpp"
#include <unordered_set>
#include <exception>

#define RB2STR(x) case x: return #x

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

static RenderingBackend_t g_eRenderingBackend = NONE;

static BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam) {
	const auto isMainWindow = [ handle ]( ) {
		return GetWindow(handle, GW_OWNER) == nullptr && IsWindowVisible(handle);
	};

	DWORD pID = 0;
	GetWindowThreadProcessId(handle, &pID);

	if (GetCurrentProcessId( ) != pID || !isMainWindow( ) || handle == GetConsoleWindow( ))
		return TRUE;

	*reinterpret_cast<HWND*>(lParam) = handle;

	return FALSE;
}

static DWORD WINAPI _UnloadDLL(LPVOID lpParam) {
	FreeLibraryAndExitThread(Utils::GetCurrentImageBase( ), 0);
	return 0;
}

namespace Utils {
	void SetRenderingBackend(RenderingBackend_t eRenderingBackground) {
		g_eRenderingBackend = eRenderingBackground;
	}

	RenderingBackend_t GetRenderingBackend( ) {
		return g_eRenderingBackend;
	}


	std::vector<std::pair<RenderingBackend_t, std::wstring>> ListLoadedRenderModules()
	{
		std::vector<std::pair<RenderingBackend_t, std::wstring>> result;

		HMODULE hMods[1024];
		DWORD cbNeeded;
		HANDLE hProcess = GetCurrentProcess();

		if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
			for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
				wchar_t szModName[MAX_PATH] = { 0 };
				if (GetModuleFileNameExW(hProcess, hMods[i], szModName, MAX_PATH)) {
					std::wstring mod = szModName;
					if (mod.find(L"d3d9.dll") != std::wstring::npos) result.emplace_back(DIRECTX9, mod);
					if (mod.find(L"d3d10.dll") != std::wstring::npos) result.emplace_back(DIRECTX10, mod);
					if (mod.find(L"d3d11.dll") != std::wstring::npos) result.emplace_back(DIRECTX11, mod);
					if (mod.find(L"d3d12.dll") != std::wstring::npos) result.emplace_back(DIRECTX12, mod);
					if (mod.find(L"opengl32.dll") != std::wstring::npos) result.emplace_back(OPENGL, mod);
					if (mod.find(L"vulkan-1.dll") != std::wstring::npos) result.emplace_back(VULKAN, mod);
				}
			}
		}
		return result;
	}
	void LogLoadedRenderModules()
	{
		auto mods = ListLoadedRenderModules();
		for (const auto& m : mods) {
			LOG_INFO_ANY("Render API module loaded: ", Logger::ws2s(m.second), " (", RenderingBackendToStr(m.first), ")");
		}
		if (mods.empty())
			LOG_WARN("No known render API modules found!");
	}
        void SetupAllHooks(HWND hWnd)
        {
                LogLoadedRenderModules();

                // Nur Hooks für tatsächlich geladene Rendering-APIs setzen
                auto modules = ListLoadedRenderModules();
                std::unordered_set<RenderingBackend_t> uniqueBackends;
                for (const auto& m : modules)
                        uniqueBackends.insert(m.first);
                LOG_DEBUG("SetupAllHooks: %zu unique backends detected", uniqueBackends.size());

                if (uniqueBackends.empty()) {
                        LOG_WARN("SetupAllHooks: No render modules detected, nothing to hook.");
                        return;
                }

                for (auto backend : uniqueBackends) {
                        try {
                                switch (backend) {
                                case DIRECTX9:
                                        LOG_INFO("DX9-Hook Attempt.");
                                        DX9::Hook(hWnd);
                                        LOG_INFO("DX9 hooked");
                                        break;
                                case DIRECTX10:
                                        LOG_INFO("DX10-Hook Attempt.");
                                        DX10::Hook(hWnd);
                                        LOG_INFO("DX10 hooked");
                                        break;
                                case DIRECTX11:
                                        LOG_INFO("DX11-Hook Attempt.");
                                        DX11::Hook(hWnd);
                                        LOG_INFO("DX11 hooked");
                                        break;
                                case DIRECTX12:
                                        LOG_INFO("DX12-Hook Attempt.");
                                        DX12::Hook(hWnd);
                                        LOG_INFO("DX12 hooked");
                                        break;
                                case OPENGL:
                                        LOG_INFO("OpenGL-Hook Attempt.");
                                        GL::Hook(hWnd);
                                        LOG_INFO("OpenGL hooked");
                                        break;
                                case VULKAN:
                                        LOG_INFO("Vulkan-Hook Attempt.");
                                        VK::Hook(hWnd);
                                        LOG_INFO("Vulkan hooked");
                                        break;
                                default:
                                        break;
                                }
                        }
                        catch (const std::exception& e) {
                                LOG_ERROR_ANY("Hooking failed for backend ", RenderingBackendToStr(backend), ": ", e.what());
                        }
                        catch (...) {
                                LOG_ERROR_ANY("Hooking failed for backend ", RenderingBackendToStr(backend));
                        }
                }

                LOG_INFO("all Hooks set.");
        }

	HWND GetProcessWindow( ) {
		HWND hwnd = nullptr;
		EnumWindows(::EnumWindowsCallback, reinterpret_cast<LPARAM>(&hwnd));

		while (!hwnd) {
			EnumWindows(::EnumWindowsCallback, reinterpret_cast<LPARAM>(&hwnd));
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

		char name[128];
		GetWindowTextA(hwnd, name, RTL_NUMBER_OF(name));

		return hwnd;
	}

	void UnloadDLL( ) {
		HANDLE hThread = CreateThread(NULL, 0, _UnloadDLL, NULL, 0, NULL);
		if (hThread != NULL) CloseHandle(hThread);
	}

	HMODULE GetCurrentImageBase( ) {
		return (HINSTANCE)(&__ImageBase);
	}

	int GetCorrectDXGIFormat(int eCurrentFormat) {
		switch (eCurrentFormat) {
			case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM;
		}

		return eCurrentFormat;
	}
}
