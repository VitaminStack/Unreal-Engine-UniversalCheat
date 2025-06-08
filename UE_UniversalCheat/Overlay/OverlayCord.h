#ifndef OVERLAYCORD_H
#define OVERLAYCORD_H

#include <Windows.h>
#include "Overlay.h"
#include <d3d11.h>
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_impl_dx11.h"
#include <iostream>
#include <tlhelp32.h>
#include <string>
#include "Font.h"


namespace OverlayCord
{
	namespace Communication
	{
		typedef struct _Header
		{
			UINT Magic;
			UINT FrameCount;
			UINT NoClue;
			UINT Width;
			UINT Height;
			BYTE Buffer[1];
		} Header;

		typedef struct _ConnectedProcessInfo
		{
			UINT ProcessId;
			HANDLE File;
			Header* MappedAddress;
		} ConnectedProcessInfo;

		bool ConnectToProcess(ConnectedProcessInfo& processInfo);
		void DisconnectFromProcess(ConnectedProcessInfo& processInfo);
		void SendFrame(ConnectedProcessInfo& processInfo, UINT width, UINT height, void* frame, UINT size);
	}

	namespace Drawing
	{
		typedef struct _Frame
		{
			UINT Width;
			UINT Height;
			UINT Size;
			void* Buffer;
		} Frame;

		typedef struct _Pixel
		{
			BYTE R, G, B, A;
		} Pixel;

		// Zeichnet ein einzelnes Zeichen auf den Framebuffer
		void DrawCharacter(Frame& frame, UINT x, UINT y, char c, Pixel color);

		// Zeichnet einen ganzen String auf den Framebuffer
		void Draw_Text(Frame& frame, UINT x, UINT y, const char* text, Pixel color, UINT spacing);

		// Zeichenfunktionen
		Frame CreateFrame(UINT width, UINT height);
		void CleanFrame(Frame& frame);
		void SetPixel(Frame& frame, UINT x, UINT y, Pixel color);
		void DrawLine(Frame& frame, UINT x1, UINT y1, UINT x2, UINT y2, Pixel color);
		void DrawRectangle(Frame& frame, UINT x1, UINT y1, UINT width, UINT height, Pixel color);
		void DrawCircle(Frame& frame, UINT x0, UINT y0, UINT radius, Pixel color);
	}
}
#endif

class ImGuiOverlay {
public:
	ImGuiOverlay(UINT width, UINT height, DWORD processId, bool debug = false);
	~ImGuiOverlay();

	void Render();
	void SendToOverlay();

private:
	UINT width, height;
	bool enableDebug;
	OverlayCord::Communication::ConnectedProcessInfo overlayProcess;

	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* context = nullptr;
	ID3D11Texture2D* renderTargetTexture = nullptr;
	ID3D11RenderTargetView* renderTargetView = nullptr;

	bool InitDirectX();
	bool InitImGui();
	bool CreateRenderTarget();
	void DebugLog(const std::string& msg);
};




HWND GetHWNDByProcessID(DWORD processID);
DWORD GetProcessIDByName(const char* exeName);
void StartRenderLoop(OverlayCord::Communication::ConnectedProcessInfo& processInfo);