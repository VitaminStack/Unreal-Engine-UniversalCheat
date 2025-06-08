#pragma once

enum RenderingBackend_t {
	NONE = 0,

	DIRECTX9,
	DIRECTX10,
	DIRECTX11,
	DIRECTX12,

	OPENGL,
	VULKAN,
};

namespace Utils {
	void SetRenderingBackend(RenderingBackend_t eRenderingBackend);
	RenderingBackend_t GetRenderingBackend( );
	inline const char* RenderingBackendToStr(RenderingBackend_t backend)
	{
		switch (backend)
		{
		case DIRECTX9:   return "DirectX 9";
		case DIRECTX10:  return "DirectX 10";
		case DIRECTX11:  return "DirectX 11";
		case DIRECTX12:  return "DirectX 12";
		case OPENGL:     return "OpenGL";
		case VULKAN:     return "Vulkan";
		default:         return "NONE/UNKNOWN";
		}
	}

	HWND GetProcessWindow( );
	void UnloadDLL( );
	
	HMODULE GetCurrentImageBase( );

	int GetCorrectDXGIFormat(int eCurrentFormat);

	void SetupAllHooks(HWND hWnd = nullptr);
}

namespace U = Utils;
